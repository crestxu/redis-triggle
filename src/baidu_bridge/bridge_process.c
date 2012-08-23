#include"redis.h"

#include"dict.h"


extern struct redisServer server;


void init_bridge_server()
{
	
     int j;

	 for (j = 0; j < server.dbnum; j++) {
        
		server.db[j].bridge_db.bridge_event= BRIDGE_DEFAULT_EVENT;
		server.db[j].bridge_db.triggle_scipts= dictCreate(&keyptrDictType,NULL);
		
		
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
	if(id<0)
		{
			addReplyError(c,"wrong dbid for triggle");
		}
	struct bridge_db_triggle_t *tmptrg=malloc(sizeof(struct bridge_db_triggle_t);
	tmptrg->dbid=id;
	tmptrg->event=int_event;
	tmptrg->lua_scripts=script_source;
	incrRefCount(script_source);
    redisDb *db = server.db[id];
	sds copy=sdsdup(key_pattern->ptr);
	dictAdd(db.bridge_db.triggle_scipts,copy,tmptrg);
    addReply(c, nx ? shared.cone : shared.ok);
	
   
}


void triggleCommand(redisClient *pc)
{

	redisClient *c=(redisClient *)pc;

    triggleGenericCommand(c,0,c->argv[1],c->argv[2],c->argv[3],c->argv[3]);
}
