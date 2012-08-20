#include"redis.h"

void do_bridge_notify(redisDb *db,redisObject *keyobj)
{
	if(db.bridge_event==BRIDGE_KEY_NOTIFY) //do notify event
		{
		    sds key = sdsnew(BRIDGE_SYSTEM_CHANNEL);
            robj *bridge_channel = createStringObject(key,sdslen(key));
			int receivers = pubsubPublishMessage(bridge_channel,keyobj);
            if (server.cluster_enabled) clusterPropagatePublish(bridge_channel,keyobj);
			
		}
}

