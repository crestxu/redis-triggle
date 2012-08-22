#include"redis.h"






void do_bridge_notify(void  *pdb,void *pkeyobj)
{
    redisDb *db=(redisDb *)pdb;
    robj *keyobj=(robj *)pkeyobj;
	if(db->bridge_event==BRIDGE_KEY_NOTIFY) //do notify event
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

     addReplyError(c,"no action for triggle");
   
}


void triggleCommand(redisClient *pc)
{

	redisClient *c=(redisClient *)pc;

    triggleGenericCommand(c,0,c->argv[1],c->argv[2],c->argv[3],c->argv[3]);
}
