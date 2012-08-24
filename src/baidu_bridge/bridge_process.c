#include"redis.h"
#include"dict.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <ctype.h>
#include <math.h>


extern struct redisServer server;
extern struct dictType keyptrDictType;

unsigned int dictSdsHash(const void *key);
int dictSdsKeyCompare(void *privdata, const void *key1, const void *key2);
void dictSdsDestructor(void *privdata, void *val);


void dicttriggleDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);
	
	struct bridge_db_triggle_t *tmptrg=(struct bridge_db_triggle_t *)val;
	decrRefCount(tmptrg->lua_scripts);
    free(val);
}


dictType keytriggleDictType = {
    dictSdsHash,               /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictSdsKeyCompare,         /* key compare */
    dictSdsDestructor,                      /* key destructor */
    dicttriggleDestructor                       /* val destructor */
};




void init_bridge_server()
{
	
     int j;

	 for (j = 0; j < server.dbnum; j++) {
        
		server.db[j].bridge_db.bridge_event= BRIDGE_DEFAULT_EVENT;
		server.db[j].bridge_db.triggle_scipts= dictCreate(&keytriggleDictType,NULL);
		
		
    }

}

void decrRefTriggleCount(void *obj) {
    robj *o = obj;

    if (o->refcount <= 0) redisPanic("decrRefCount against refcount <= 0");
    if (o->refcount == 1) {
        switch(o->type) {
        case REDIS_STRING: freeStringObject(o); break;
        case REDIS_LIST: freeListObject(o); break;
        case REDIS_SET: freeSetObject(o); break;
        case REDIS_ZSET: freeZsetObject(o); break;
        case REDIS_HASH: freeHashObject(o); break;
        default: redisPanic("Unknown object type"); break;
        }
        zfree(o);
    } else {
        o->refcount--;
    }
}


robj *createTriggleObject(void) {
    list *l = listCreate();
    robj *o = createObject(REDIS_LIST,l);
    listSetFreeMethod(l,decrRefTriggleCount);
    o->encoding = REDIS_ENCODING_RAW;
    return o;
}


void do_bridge_notify(void  *pdb,void *pkeyobj)
{
    redisDb *db=(redisDb *)pdb;
    robj *keyobj=(robj *)pkeyobj;
	if(db->bridge_db.bridge_event==BRIDGE_KEY_NOTIFY) //do notify event
		{
		    sds key = sdsnew(BRIDGE_SYSTEM_CHANNEL);
            robj *bridge_channel = createStringObject(key,sdslen(key));
			int receivers = pubsubPublishMessage(bridge_channel,keyobj);
            if (server.cluster_enabled) clusterPropagatePublish(bridge_channel,keyobj);
		    redisLog(REDIS_NOTICE,"%d clients receive the expire event",receivers);
            decrRefCount(bridge_channel);
		}
}



void triggleGenericCommand(redisClient *c, int nx, robj *db_id, robj *key_pattern,robj *event_type, robj *script_source) {
//    long long milliseconds = 0; /* initialized to avoid an harmness warning */

   /* if (expire) {
        if (getLongLongFromObjectOrReply(c, expire, &milliseconds, NULL) != REDIS_OK)
            return;
        if (milliseconds <= 0) {
            addReplyError(c,"invalid expire time in SETEX");
            return;
        }
        if (unit == UNIT_SECONDS) milliseconds *= 1000;
    }

    if (lookupKeyWrite(c->db,key) != NULL && nx) {
        addReply(c,shared.czero);
        return;
    }
    setKey(c->db,key,val);
    server.dirty++;
    if (expire) setExpire(c->db,key,mstime()+milliseconds);
    addReply(c, nx ? shared.cone : shared.ok);*/
    redisLog(REDIS_NOTICE,"dbid: %s keypattern: %s script_source: %s ",db_id->ptr,key_pattern->ptr,script_source->ptr);
   int id = atoi(db_id->ptr);
	int int_event=atoi(event_type->ptr);
	if(id<0||id>server.dbnum)
		{
			addReplyError(c,"wrong dbid for triggle");
            return;
		}


   /*lua_check*/
   sds funcdef = sdsempty();

   funcdef = sdscat(funcdef,"function ");
   funcdef = sdscatlen(funcdef,key_pattern->ptr,sdslen(key_pattern->ptr));
   funcdef = sdscatlen(funcdef,"() ",3);
   funcdef = sdscatlen(funcdef,script_source->ptr,sdslen(script_source->ptr));
   funcdef = sdscatlen(funcdef," end",4);
   redisLog(REDIS_NOTICE,"script function:%s",funcdef);
   
   if (luaL_loadbuffer(server.lua,funcdef,sdslen(funcdef),"@user_script")) {
		   addReplyErrorFormat(c,"Error compiling script (new function): %s\n",
			   lua_tostring(server.lua,-1));
		   lua_pop(server.lua,1);
		   sdsfree(funcdef);
		   return ;
   }
   sdsfree(funcdef);
   if (lua_pcall(server.lua,0,0,0)) {
		   addReplyErrorFormat(c,"Error running script (new function): %s\n",
			   lua_tostring(server.lua,-1));
		   lua_pop(server.lua,1);
		   return ;
   }


   /*end lua check*/



	struct bridge_db_triggle_t *tmptrg=malloc(sizeof(struct bridge_db_triggle_t));
	tmptrg->dbid=id;
	tmptrg->event=int_event;
	tmptrg->lua_scripts=script_source;
	incrRefCount(script_source);
    sds copy=sdsdup(key_pattern->ptr);
    dictAdd(server.db[id].bridge_db.triggle_scipts,copy,tmptrg);
    addReply(c, nx ? shared.cone : shared.ok);
	
   
}

void triggleDelCommand(struct redisClient *c)
{
	int id = atoi(c->argv[1]->ptr);
		if(id<0||id>server.dbnum)
			{
				addReplyError(c,"wrong dbid for triggle");
				return;
			}
		if(	dictDelete(server.db[id].bridge_db.triggle_scipts,c->argv[2]->ptr)==DICT_OK)
		{
		   addReply(c,  shared.ok);
	
		}
		else
		{
			addReplyError(c,"delete unknow error");
		}
		



}


void triggleListCommand(struct redisClient *c)
{
  
    int id = atoi(c->argv[1]->ptr);
	if(id<0||id>server.dbnum)
		{
			addReplyError(c,"wrong dbid for triggle");
            return;
		}
	//struct bridge_db_triggle_t *tmptrg=malloc(sizeof(struct bridge_db_triggle_t));
	//tmptrg->dbid=id;
	//tmptrg->event=int_event;
	//tmptrg->lua_scripts=script_source;
	//incrRefCount(script_source);
    //sds copy=sdsdup(key_pattern->ptr);
    //dictAdd(server.db[id].bridge_db.triggle_scipts,copy,tmptrg);
    redisLog(REDIS_NOTICE,"dbid:%d key:%s",id,c->argv[2]->ptr);
    struct dictEntry *de = dictFind(server.db[id].bridge_db.triggle_scipts,c->argv[2]->ptr);
    if(de)
    {
      
       struct bridge_db_triggle_t * tmptrg=dictGetVal(de);
      
        addReplyStatusFormat(c,"dbid:%dkey:%sevent:%d source:%s",tmptrg->dbid,c->argv[2]->ptr,tmptrg->event,tmptrg->lua_scripts->ptr);

    }
    else
    {
        addReplyError(c,"triggle not found");
    }
    //addReply(c, nx ? shared.cone : shared.ok);
	
}


void triggleCommand(redisClient *pc)
{

	redisClient *c=(redisClient *)pc;

    triggleGenericCommand(c,0,c->argv[1],c->argv[2],c->argv[3],c->argv[4]);
}

void luaReplyToLog(redisClient *c, lua_State *lua) {
    int t = lua_type(lua,-1);

    switch(t) {
    case LUA_TSTRING:
        addReplyBulkCBuffer(c,(char*)lua_tostring(lua,-1),lua_strlen(lua,-1));
        break;
    case LUA_TBOOLEAN:
        addReply(c,lua_toboolean(lua,-1) ? shared.cone : shared.nullbulk);
        break;
    case LUA_TNUMBER:
        addReplyLongLong(c,(long long)lua_tonumber(lua,-1));
        break;
    case LUA_TTABLE:
        /* We need to check if it is an array, an error, or a status reply.
         * Error are returned as a single element table with 'err' field.
         * Status replies are returned as single elment table with 'ok' field */
        lua_pushstring(lua,"err");
        lua_gettable(lua,-2);
        t = lua_type(lua,-1);
        if (t == LUA_TSTRING) {
            sds err = sdsnew(lua_tostring(lua,-1));
            sdsmapchars(err,"\r\n","  ",2);
            addReplySds(c,sdscatprintf(sdsempty(),"-%s\r\n",err));
            sdsfree(err);
            lua_pop(lua,2);
            return;
        }

        lua_pop(lua,1);
        lua_pushstring(lua,"ok");
        lua_gettable(lua,-2);
        t = lua_type(lua,-1);
        if (t == LUA_TSTRING) {
            sds ok = sdsnew(lua_tostring(lua,-1));
            sdsmapchars(ok,"\r\n","  ",2);
            addReplySds(c,sdscatprintf(sdsempty(),"+%s\r\n",ok));
            sdsfree(ok);
            lua_pop(lua,1);
        } else {
            void *replylen = addDeferredMultiBulkLength(c);
            int j = 1, mbulklen = 0;

            lua_pop(lua,1); /* Discard the 'ok' field value we popped */
            while(1) {
                lua_pushnumber(lua,j++);
                lua_gettable(lua,-2);
                t = lua_type(lua,-1);
                if (t == LUA_TNIL) {
                    lua_pop(lua,1);
                    break;
                }
                luaReplyToRedisReply(c, lua);
                mbulklen++;
            }
            setDeferredMultiBulkLength(c,replylen,mbulklen);
        }
        break;
    default:
        addReply(c,shared.nullbulk);
    }
    lua_pop(lua,1);
}

int do_delete_event(struct redisClient *c,sds *funcname)
{
	luaSetGlobalArray(server.lua,"KEYS",c->argv+1,c->argc-1);
	
   // luaSetGlobalArray(server.lua,"ARGV",c->argv+3+numkeys,c->argc-3-numkeys);

    /* Select the right DB in the context of the Lua client */
    selectDb(server.lua_client,c->db->id);


   if (server.lua_time_limit > 0) {
        lua_sethook(server.lua,luaMaskCountHook,LUA_MASKCOUNT,100000);
        server.lua_time_start = ustime()/1000;
    } else {
        lua_sethook(server.lua,luaMaskCountHook,0,0);
    }

    /* At this point whatever this script was never seen before or if it was
     * already defined, we can call it. We have zero arguments and expect
     * a single return value. */
    if (lua_pcall(server.lua,0,1,0)) {
        selectDb(c,server.lua_client->db->id); /* set DB ID from Lua client */
        addReplyErrorFormat(c,"Error running script (call to %s): %s\n",
            funcname, lua_tostring(server.lua,-1));
        lua_pop(server.lua,1);
        lua_gc(server.lua,LUA_GCCOLLECT,0);
        return;
    }
    selectDb(c,server.lua_client->db->id); /* set DB ID from Lua client */
   // luaReplyToRedisReply(c,lua);
    lua_gc(server.lua,LUA_GCSTEP,1);
   
}


void call_bridge_event(struct redisClient *c,int event_type)
{
	dictIterator *iter=dictGetIterator(c->db.bridge_db.triggle_scripts);
    dictEntry *trigs;
    do{
    	trigs=dictNext(iter);
		struct bridge_db_triggle_t * tmptrg=dictGetVal(trigs);
		if(tmptrg==event_type){ //找到指定的类型事件
		
		    //do lua call
		    //check the keys allows first
		    switch(event_type){
				case DELETE_EVENT: 
					do_delete_event(c,dictGetKey(trigs);)
					break;
				default:
					break;
				
		    }
		}
    }while(trigs!=NULL);
	dictRelease(iter);

    
}

