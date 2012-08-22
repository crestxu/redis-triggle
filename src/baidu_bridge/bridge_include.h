#ifndef BAIDU_BRIDGE
#define BAIDU_BRIDGE

#define BRIDGE_SYSTEM_CHANNEL "system"  //define the system default channel name to suscribe the system event
#define BRIDGE_DEFAULT_EVENT 1  //define default delete event when the keys expires
#define BRIDGE_KEY_NOTIFY 1
#define BRIDGE_KEY_UNNOTIFY 0 

#define BRIDGE_DB_EXTENTIONS bridge_db_externtion bridge_db;


struct bridge_db_triggle;

struct bridge_db_externtion;

enum BAIDU_BRIDGE_TRIGGLE{
	/*General keys operation*/
	DELETE_EVENT=203, //delete command triggle
	EXPIRED_EVENT,  //when expired the key from the database triggle
	/*String keys*/
	SET_EVENT,
	MSET_EVENT,
	SETEX_EVENT,
	SETBIT_EVENT,
	/*HASHES*/
	HDEL_EVENT,
	HSET_EVENT,
	/*LIST*/
	LPUSH_EVENT,
	LPOP_EVENT,
	LREM_EVENT,
	LSET_EVENT,
	RPUSH_EVENT,
	RPOP_EVENT,
	/*SET*/
	SADD_EVENT,
	SREM_EVENT,
	/*ZSET*/
	ZADD_EVENT,
	ZREM_EVENT,
	/*PUB,SUB*/
	PUBLISH_EVENT
};



void do_bridge_notify(void *db,void *keyobj);

void triggleCommand(void *c); //redisClient *c





#endif
