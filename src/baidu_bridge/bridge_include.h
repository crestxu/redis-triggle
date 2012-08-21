#ifndef BAIDU_BRIDGE
#define BAIDU_BRIDGE

#define BRIDGE_SYSTEM_CHANNEL "system"  //define the system default channel name to suscribe the system event
#define BRIDGE_DEFAULT_EVENT 1  //define default delete event when the keys expires
#define BRIDGE_KEY_NOTIFY 1
#define BRIDGE_KEY_UNNOTIFY 0 


void do_bridge_notify(void *db,void *keyobj);

void triggleCommand(void *c); //redisClient *c





#endif
