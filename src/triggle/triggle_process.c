#include"redis.h"
#include"dict.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <ctype.h>
#include <math.h>


void redisSrand48(int32_t seedval) ;
void luaSetGlobalArray(lua_State *lua, char *var, robj **elev, int elec);
struct triggleCmd{
    enum BAIDU_BRIDGE_TRIGGLE event;
    char *name;
};


struct triggleCmd triggleCmdTable[]={
    {DELETING_EVENT,"deling"}, //delete command triggle
    {DELETED_EVENT,"deled"},

    {SETING_EVENT,"seting"},
    {SETED_EVENT,"seted"},


    {SETNXING_EVENT,"setnxing"},
    {SETNXED_EVENT,"setnxed"},


    {SETEXING_EVENT,"setexing"},
    {SETEXED_EVENT,"setexed"},





    {EXPIRING_EVENT,"expiring"},
    {EXPIRED_EVENT, "expired"}, //when expired the key from the database triggle
    /*String keys*/

    {MSETING_EVENT,"mseting"},

    {MSETED_EVENT,"mseted"},


    {SETBITING_EVENT,"setbiting"},

    {SETBITED_EVENT,"setbited"},

    {HDELING_EVENT,"hdeling"},

    {HDELED_EVENT,"hdeled"},
    {HSETING_EVENT,"hseting"},

    {HSETED_EVENT,"hseted"},

    {LPUSHING_EVENT,"lpushing"},

    {LPUSHED_EVENT,"lpushed"},
    {LPOPING_EVENT,"lpoping"},
    {LPOPED_EVENT,"lpoped"},

    {LREMING_EVENT,"lreming"},

    {LREMED_EVENT,"lremed"},


    {LSETING_EVENT,"lseting"},

    {LSETED_EVENT,"lseted"},

    {RPUSHING_EVENT,"rpushing"},

    {RPUSHED_EVENT,"rpushed"},

    {RPOPING_EVENT,"rpoping"},



    {SADDING_EVENT,"sadding"},

    {SADDED_EVENT,"sadded"},
    {SREMING_EVENT,"sreming"},

    {SREMED_EVENT,"sremed"},
    /*ZSET*/
    {ZADDING_EVENT,"zadding"},

    {ZADDED_EVENT,"zadded"},

    {ZREMING_EVENT,"zreming"},

    {ZREMED_EVENT,"zremed"},
    /*PUB,SUB*/
    {PUBLISH_EVENT,"publish"},

    {DELETE_EXPIRE,"delete_expire"}
};

extern struct redisServer server;
extern struct dictType keyptrDictType;

unsigned int dictSdsHash(const void *key);
int dictSdsKeyCompare(void *privdata, const void *key1, const void *key2);
void dictSdsDestructor(void *privdata, void *val);
void luaMaskCountHook(lua_State *lua, lua_Debug *ar);

void dicttriggleDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);

    struct bridge_db_triggle_t *tmptrg=(struct bridge_db_triggle_t *)val;
    decrRefCount(tmptrg->lua_scripts);
    zfree(val);
}


dictType keytriggleDictType = {
    dictSdsHash,               /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictSdsKeyCompare,         /* key compare */
    dictSdsDestructor,                      /* key destructor */
    dicttriggleDestructor                       /* val destructor */
};






int luatriggleCreateFunction(lua_State *lua, sds funcname, robj *body) {
    sds funcdef = sdsempty();

    funcdef = sdscat(funcdef,"function ");
    funcdef = sdscatlen(funcdef,funcname,sdslen(funcname));
    funcdef = sdscatlen(funcdef,"() ",3);
    funcdef = sdscatlen(funcdef,body->ptr,sdslen(body->ptr));
    funcdef = sdscatlen(funcdef," end",4);

    redisLog(REDIS_NOTICE,"create lua function start: %s",funcdef);
    if (luaL_loadbuffer(lua,funcdef,sdslen(funcdef),"@user_script")) {
        redisLog(REDIS_WARNING,"Error compiling script (new function): %s\n",
                lua_tostring(lua,-1));
        lua_pop(lua,1);
        sdsfree(funcdef);
        return REDIS_ERR;
    }
    sdsfree(funcdef);

    redisLog(REDIS_NOTICE,"Load buffer ok !create lua function start: %s",funcdef);
    if (lua_pcall(lua,0,0,0)) {
        redisLog(REDIS_WARNING,"Error running script (new function): %s\n",
                lua_tostring(lua,-1));
        lua_pop(lua,1);
        return REDIS_ERR;
    }

    redisLog(REDIS_NOTICE,"call ok !create lua function start: %s",funcdef);

    return REDIS_OK;
}

void loadtriggleCmds()
{


    int num=sizeof(triggleCmdTable)/sizeof(struct triggleCmd);
    int i;
    redisLog(REDIS_NOTICE,"num :%d",num);
    for(i=0;i<num;i++)
    {

        struct triggleCmd *c=&triggleCmdTable[i];
        redisLog(REDIS_NOTICE,"name:%s event:%d",c->name,c->event);
        dictAdd(server.bridge_db.triggle_cmds,sdsnew(c->name), c);
    }

}


int process_trigglecmd(sds name)
{

    //int i=0;
    struct triggleCmd *p=(struct triggleCmd *)dictFetchValue(server.bridge_db.triggle_cmds, name);



    return (p==NULL)?-1:p->event; 
}

void init_bridge_server()
{



    server.bridge_db.bridge_event=zmalloc(sizeof(int)*server.dbnum);

    server.bridge_db.triggle_cmds= dictCreate(&keyptrDictType,NULL);
    server.bridge_db.triggle_scipts=zmalloc(sizeof(dict *)*server.dbnum);
    int j;

    for (j = 0; j < server.dbnum; j++) {

        server.bridge_db.bridge_event[j]= BRIDGE_DEFAULT_EVENT;
        server.bridge_db.triggle_scipts[j]= dictCreate(&keytriggleDictType,NULL);


    }
    loadtriggleCmds();

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
    if(server.bridge_db.bridge_event[db->id]==BRIDGE_KEY_NOTIFY) //do notify event
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
    redisLog(REDIS_NOTICE,"dbid: %s keypattern: %s script_source: %s ",db_id->ptr,key_pattern->ptr,script_source->ptr);
    int id = atoi(db_id->ptr);
    int int_event=process_trigglecmd(event_type->ptr);
    if(int_event==-1)
    {
        addReplyError(c,"undefine event in redis triggle");
        return;
    }


    
    redisLog(REDIS_NOTICE,"get event:%d for: %s",int_event,event_type->ptr);
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

   lua_getglobal(server.lua,key_pattern->ptr);
   if (lua_isnil(server.lua,1)) {
        lua_pop(server.lua,1);
        redisLog(REDIS_WARNING,"No function define in lua:%s",key_pattern->ptr);
   }
    /*end lua check*/


    struct bridge_db_triggle_t *tmptrg=zmalloc(sizeof(struct bridge_db_triggle_t));
    tmptrg->dbid=id;
    tmptrg->event=int_event;
    tmptrg->lua_scripts=script_source;
    incrRefCount(script_source);
    sds copy=sdsdup(key_pattern->ptr);
    dictAdd(server.bridge_db.triggle_scipts[id],copy,tmptrg);
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
    if(	dictDelete(server.bridge_db.triggle_scipts[id],c->argv[2]->ptr)==DICT_OK)
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
    redisLog(REDIS_NOTICE,"dbid:%d key:%s",id,c->argv[2]->ptr);
    struct dictEntry *de = dictFind(server.bridge_db.triggle_scipts[id],c->argv[2]->ptr);
    if(de)
    {

        struct bridge_db_triggle_t * tmptrg=dictGetVal(de);

        addReplyStatusFormat(c,"dbid:%dkey:%sevent:%d source:%s",tmptrg->dbid,(char *)c->argv[2]->ptr,tmptrg->event,(char *)tmptrg->lua_scripts->ptr);

    }
    else
    {
        addReplyError(c,"triggle not found");
    }

}


void triggleCommand(redisClient *pc)
{

    redisClient *c=(redisClient *)pc;

    triggleGenericCommand(c,0,c->argv[1],c->argv[2],c->argv[3],c->argv[4]);
}


int do_delete_event(struct redisClient *c,sds funcname)
{

    redisSrand48(0);
    server.lua_random_dirty = 0;
    server.lua_write_dirty = 0;
    lua_getglobal(server.lua,(char *)funcname);



    if (lua_isnil(server.lua,1)) {
        addReplyError(c,"no funcname triggle_scipts in lua");
        return 0;
    }
    luaSetGlobalArray(server.lua,"KEYS",c->argv+1,c->argc-1);

    redisLog(REDIS_NOTICE,"stack: %d",lua_gettop(server.lua));


#ifdef BRIDGE_DEBUG
    for(int i=0;i<c->argc-1;i++){
        redisLog(REDIS_NOTICE,"%s",(c->argv+1)[i]->ptr);
    }
#endif


    selectDb(server.lua_client,c->db->id);


    server.lua_time_start = ustime()/1000;
    server.lua_kill = 0;

    if (server.lua_time_limit > 0) {
        lua_sethook(server.lua,luaMaskCountHook,LUA_MASKCOUNT,100000);
        server.lua_time_start = ustime()/1000;
    } else {
        lua_sethook(server.lua,luaMaskCountHook,0,0);
    }

    if (lua_pcall(server.lua,0,1,0)) {
        selectDb(c,server.lua_client->db->id); 
        addReplyErrorFormat(c,"Error running script (call to %s): %s\n",
                (char*)funcname, lua_tostring(server.lua,-1));
        lua_pop(server.lua,1);
        lua_gc(server.lua,LUA_GCCOLLECT,0);
        return -1;
    }
    selectDb(c,server.lua_client->db->id); 
    // luaReplyToRedisReply(c,server.lua);
    server.lua_timedout = 0;
    server.lua_caller = NULL;
    lua_gc(server.lua,LUA_GCSTEP,1);

    //for slaves
    //

    return 0; 
}


int triggle_event(struct redisClient *c,sds funcname)
{

    redisSrand48(0);
    server.lua_random_dirty = 0;
    server.lua_write_dirty = 0;
    lua_getglobal(server.lua,funcname);



    if (lua_isnil(server.lua,1)) {
        lua_pop(server.lua,1); /* remove the nil from the stack */

        redisLog(REDIS_NOTICE,"no funcname triggle_scipts in lua");

        struct dictEntry *de = dictFind(server.bridge_db.triggle_scipts[c->db->id],funcname);
        if(de)
        {

            struct bridge_db_triggle_t * tmptrg=dictGetVal(de);
            if (luatriggleCreateFunction(server.lua,funcname,tmptrg->lua_scripts) == REDIS_ERR) return -1;
            /* Now the following is guaranteed to return non nil */
            lua_getglobal(server.lua, funcname);
            redisAssert(!lua_isnil(server.lua,1));


        }
        else
        {
            redisLog(REDIS_WARNING,"triggle not found");
            return -1;
        }


    }
    luaSetGlobalArray(server.lua,"KEYS",c->argv,c->argc);

    redisLog(REDIS_NOTICE,"stack: %d",lua_gettop(server.lua));


#ifdef BRIDGE_DEBUG
    for(int i=0;i<c->argc;i++){
        redisLog(REDIS_NOTICE,"%s",c->argv[i]->ptr);
    }
#endif


    /* Select the right DB in the context of the Lua client */
    selectDb(server.lua_client,c->db->id);


    server.lua_time_start = ustime()/1000;
    server.lua_kill = 0;

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
        redisLog(REDIS_WARNING,"Error running script (call to %s): %s\n",
                (char*)funcname, lua_tostring(server.lua,-1));
        lua_pop(server.lua,1);
        lua_gc(server.lua,LUA_GCCOLLECT,0);
        return -1;
    }
    selectDb(c,server.lua_client->db->id); /* set DB ID from Lua client */
    // luaReplyToRedisReply(c,server.lua);
    server.lua_timedout = 0;
    server.lua_caller = NULL;
    lua_gc(server.lua,LUA_GCSTEP,1);

    redisLog(REDIS_NOTICE,"after stack: %d",lua_gettop(server.lua));
    //for slaves
    //

    return 0; 
}

void call_bridge_event(struct redisClient *c,int triggle_place,int event_type)
{

    if(event_type==-1)
    {
        sds cmds=(sds)sdsdup(c->argv[0]->ptr);

        if(triggle_place==TRIGGLE_BEFORE) //no event given
        {
            cmds=sdscat(cmds,"ing");


        }
        if(triggle_place==TRIGGLE_AFTER)
        {
            cmds=sdscat(cmds,"ed");

        }

        event_type=process_trigglecmd(cmds);

  //      redisLog(REDIS_NOTICE,"cmds:%s id:%d",cmds,event_type);
        sdsfree(cmds);
    }

    if(event_type==-1)
        return;
    struct dictIterator *iter=dictGetIterator(server.bridge_db.triggle_scipts[c->db->id]);
    dictEntry *trigs;
    do{
        trigs=dictNext(iter);
        if(trigs!=NULL)
        {
            struct bridge_db_triggle_t * tmptrg=dictGetVal(trigs);
            if(tmptrg->event==event_type){ //找到指定的类型事件
//                redisLog(REDIS_NOTICE,"triggle_event:%d,%s",event_type,(char *)dictGetKey(trigs));
                triggle_event(c,dictGetKey(trigs));
            }
        }
    }while(trigs!=NULL);
    dictReleaseIterator(iter);


}
void call_bridge_event_before(struct redisClient *c,int event_type)
{
    call_bridge_event(c,TRIGGLE_BEFORE,event_type);
}


void call_bridge_event_after(struct redisClient *c,int event_type)
{

    call_bridge_event(c,TRIGGLE_AFTER,event_type);
}

int triggle_expire_event(redisDb *db,sds funcname,robj *key)
{

    redisSrand48(0);
    server.lua_random_dirty = 0;
    server.lua_write_dirty = 0;
    lua_getglobal(server.lua,funcname);



    if (lua_isnil(server.lua,1)) {
        lua_pop(server.lua,1); /* remove the nil from the stack */

        redisLog(REDIS_NOTICE,"no funcname triggle_scipts in lua");

        struct dictEntry *de = dictFind(server.bridge_db.triggle_scipts[db->id],funcname);
        if(de)
        {

            struct bridge_db_triggle_t * tmptrg=dictGetVal(de);
            if (luatriggleCreateFunction(server.lua,funcname,tmptrg->lua_scripts) == REDIS_ERR) return -1;
            /* Now the following is guaranteed to return non nil */
            lua_getglobal(server.lua, funcname);
            redisAssert(!lua_isnil(server.lua,1));


        }
        else
        {
            redisLog(REDIS_WARNING,"triggle not found");
            return -1;
        }

    }
    luaSetGlobalArray(server.lua,"KEYS",&key,1);




    server.lua_time_start = ustime()/1000;
    server.lua_kill = 0;

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
        //selectDb(c,server.lua_client->db->id); /* set DB ID from Lua client */

        redisLog(REDIS_WARNING,"exec the script error:%s",lua_tostring(server.lua,-1));
        lua_pop(server.lua,1);
        lua_gc(server.lua,LUA_GCCOLLECT,0);
        return -1;
    }
    //selectDb(c,server.lua_client->db->id); /* set DB ID from Lua client */
    server.lua_timedout = 0;
    server.lua_caller = NULL;
    lua_gc(server.lua,LUA_GCSTEP,1);


    return 0; 
}


void call_expire_delete_event(void *pdb,void *pkeyobj)
{
    redisDb *db=(redisDb *)pdb;
    robj *myobj=(robj *)pkeyobj;
    robj *keyobj = createStringObject(myobj->ptr,sdslen(myobj->ptr));

    struct dictIterator *iter=dictGetIterator(server.bridge_db.triggle_scipts[db->id]);
    dictEntry *trigs;
    do{
        trigs=dictNext(iter);
        if(trigs!=NULL)
        {
            struct bridge_db_triggle_t * tmptrg=dictGetVal(trigs);
            if(tmptrg->event==DELETE_EXPIRE){ //找到指定的类型事件
                redisLog(REDIS_NOTICE,"triggle_event:%d,%s",DELETE_EXPIRE,(char *)dictGetKey(trigs));
                triggle_expire_event(db,dictGetKey(trigs),keyobj);
            }
        }
    }while(trigs!=NULL);
    dictReleaseIterator(iter);

    decrRefCount(keyobj);  

}


void rdb_save_triggles(rio *rdb)
{

    //save event 
    //db_num int int int int 
    //db 
    //scripts_num
    //key event lua_scripts 
    //key event lua_scripts 
    //.......
    dictIterator *di = NULL;
    dictEntry *de;   
    int i=0;
    for(i=0;i<server.dbnum;i++){
        int eventid=server.bridge_db.bridge_event[i]; 
        rioWrite(rdb,&eventid,4);
    }
    for(i=0;i<server.dbnum;i++)
    {
        dict *d = server.bridge_db.triggle_scipts[i];
        int mysize=dictSize(d);
        rioWrite(rdb,&mysize,4);
        if (dictSize(d) == 0) continue;
        di = dictGetSafeIterator(d);
        if (!di) {
            return ;
        }
        /* Iterate this DB writing every entry */
        while((de = dictNext(di)) != NULL) {
            sds keystr = dictGetKey(de);
            robj key;
            initStaticStringObject(key,keystr);
            if (rdbSaveStringObject(rdb,&key) == -1) return;

            struct bridge_db_triggle_t * tmptrg=dictGetVal(de);
            int event_id=tmptrg->event; 
            rioWrite(rdb,&event_id,4);
            int db_id=tmptrg->dbid; 
            rioWrite(rdb,&db_id,4);
            if (rdbSaveObjectType(rdb,tmptrg->lua_scripts) == -1) return ;
            if (rdbSaveObject(rdb,tmptrg->lua_scripts) == -1) return ; 
        }
    }
    if (di) dictReleaseIterator(di);
}

void rdb_load_triggle(rio *rdb)
{
    //save event 
    //db_num int int int int 
    //db 
    //scripts_num
    //key event lua_scripts 
    //key event lua_scripts 
    //.......
    int i=0;
    for(i=0;i<server.dbnum;i++){
        int eventid=rioRead(rdb,&eventid,4);
        server.bridge_db.bridge_event[i]=eventid;
//        printf('read from dump file eventid:%d',eventid);
    }
    long j=0;
    for(i=0;i<server.dbnum;i++)
    {

        
        int dsize;
        rioRead(rdb,&dsize,4);
    //    redisLog(REDIS_NOTICE,"read triggle dbsize, dbid:%d size:%d",i,dsize);
        for(j=0;j<dsize;j++)
        {
             struct bridge_db_triggle_t *tmptrg=zmalloc(sizeof(struct bridge_db_triggle_t));
            
            robj* key=rdbLoadStringObject(rdb);
            
            rioRead(rdb,&tmptrg->event,4);
            rioRead(rdb,&tmptrg->dbid,4);
            int type;
            type=rdbLoadType(rdb);
            robj *mm=rdbLoadObject(type,rdb);
            tmptrg->lua_scripts=mm;

      //      redisLog(REDIS_NOTICE,"redis one triggle row, key:%s event:%d db:%d script:%s",copy,tmptrg->event,tmptrg->dbid,tmptrg->lua_scripts->ptr);

            sds copy=sdsdup(key->ptr);
            dictAdd(server.bridge_db.triggle_scipts[i],copy,tmptrg);
            decrRefCount(key);



        }
        
    }

}


