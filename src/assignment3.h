#include <stdint.h>
struct init {
   uint32_t type; 
   uint32_t version;
   char name[255];
   
} __attribute__((packed));
struct request {
   uint32_t data_len; 
   uint8_t type;
} __attribute__((packed));
struct nickname{
   uint8_t data_len; 
   char name[255];
} __attribute__((packed));
struct clnt{
   struct clnt *next;
   struct clnt *prev;
   int clntsocket;
   int rand;
   char name[255];
   char room[255];
}; 
struct chatroom{
   struct chatroom *next;
   struct chatroom *prev;
   uint8_t n_clients;
   char name[255];
   char pass[255];
};
struct chatroom_req{
	uint8_t name_len;
	uint8_t pass_len;
	char pass[255];
	char name[255];
}__attribute__((packed));
struct message_req{
	uint8_t name_len;
	uint8_t msg_len;
	uint8_t room_len;
	char name[255];
	char room[255];
	char msg[65535];	
}__attribute__((packed));
struct ThreadArgs{
    int clntSock;                      /* Socket descriptor for client */
};  
void deallocate_mem();

int create_clntls();
struct clnt *get_globalclnts();
struct clnt *add_clnt(int clntsocket);
void print_clnt(struct clnt* client);
void print_clnts();
struct clnt *find_client(char name [255]);
int del_client(char name[255]);



int create_rooms();
int add_room(char name [255],char pass[255]);
void print_room(struct chatroom* room);
void print_rooms();
struct chatroom *find_room(char name [255]);
int del_room(char name[255]);
struct chatroom *get_globalrooms();
void sort_rooms();
