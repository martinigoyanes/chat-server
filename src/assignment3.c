#include "queue.h"
#include "assignment3.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
static struct clnt *clnts_gl;
static struct chatroom *room_gl;
struct clnt *get_globalclnts(){
	return clnts_gl;
}
int create_clntls()
{
	InitDQ(clnts_gl, struct clnt);
	assert (clnts_gl);
  
	bzero(clnts_gl->room,256);
	bzero(clnts_gl->name,256);
  
	return (clnts_gl != 0x0);
}
struct clnt *add_clnt(int clntsocket)
{
	struct clnt* client = (struct clnt *) getmem (sizeof (struct clnt));
	int next_aval=0;
	char *tempname = malloc(4*sizeof(char) + sizeof(int));
	
	struct clnt *i;
	for (i = clnts_gl->prev; i != clnts_gl; i= i->prev){
		sprintf(tempname,"rand%d",next_aval);
		if(strcmp(tempname,i->name) == 0){
			next_aval++;
			i=clnts_gl->prev;
		}
		else{
			sprintf(tempname,"rand%d",next_aval);
		}
		
	}
	sprintf(tempname,"rand%d",next_aval);
	
	bzero(client->room,256);
	strcpy(client->name,tempname);
	client->clntsocket = clntsocket;
	
	free(tempname);
	tempname=NULL;
	
	InsertDQ(clnts_gl, client);
	return client;
}
void print_clnt(struct clnt* client)
{
	fprintf (stdout, "[clnt]\t ----- client name(%s) ----- client room(%s)\n", client->name,client->room);
}
void print_clnts()
{
	struct clnt *client;

	fprintf (stdout, "\n[client] ***** dumping clients set *****\n");
	for (client=clnts_gl->next; client != clnts_gl; client=client->next){
		assert(client);
		print_clnt (client);
	}
}
struct clnt *find_client(char name [255])
{
	struct clnt *i;
	for (i = clnts_gl->next; i != clnts_gl; i= i->next){
		if (!(strcmp(i->name, name))){
			break;
		}
	}
	if (!strcmp(i->name, name))
	{
		return i;
	}
	else    {
		return 0x0;
	}
}
int del_client(char name[255])
{
	struct clnt *i = find_client(name);
	i->clntsocket = 0;
	bzero(i->name,256);
	bzero(i->room,256);
	assert (i);
	DelDQ(i);
	free(i);
	return 0x0;
}
int create_rooms()
{
	InitDQ(room_gl, struct chatroom);
	assert (room_gl);
  
	
	bzero(room_gl->name,256);
	bzero(room_gl->pass,256);
  
	return (room_gl != 0x0);
}
struct chatroom *get_globalrooms()
{
	return room_gl;
}
int add_room(char name [255],char pass[255])
{
	struct chatroom* room = (struct chatroom *) getmem (sizeof (struct chatroom));
	
	strcpy(room->name,&name[0]);
	strcpy(room->pass,&pass[0]);
	room->n_clients = 1;
		
	
	InsertDQ(room_gl, room);
	sort_rooms();
	return (room != 0x0);
}

void print_room(struct chatroom* room)
{
	fprintf (stdout, "[%s]\t ----- room people(%d) ----- room pass(%s)\n", room->name,room->n_clients,room->pass);
}
void print_rooms()
{
	struct chatroom *room;

	fprintf (stdout, "\n[rooms] ***** dumping room set *****\n");
	for (room=room_gl->next; room != room_gl; room=room->next){
		assert(room);
		print_room (room);
	}
}
struct chatroom *find_room(char name [255])
{
	struct chatroom *i;
	for (i = room_gl->next; i != room_gl; i= i->next){
		if (!(strcmp(i->name, name))){
			break;
		}
	}
	if (!strcmp(i->name, name))
	{
		return i;
	}
	else    {
		return 0x0;
	}
}
int del_room(char name[255])
{
	struct chatroom *i = find_room(name);
	i->n_clients = 0;
	bzero(i->name,256);
	bzero(i->pass,256);
	assert (i);
	DelDQ(i);
	free(i);
	sort_rooms();
	return 0x0;
}
void sort_rooms(){
	
	struct chatroom *i,*b;
	uint8_t  n_clients=0;
	char name [255];
	char pass [255];
	
	bzero(name,255);
	bzero(pass,255);
	
	
	for (b = room_gl->next; b != room_gl; b= b->next){
		for (i = b->next; i != room_gl; i= i->next){
			if (strcmp(b->name, i->name)>0 ){
				strcpy(name,i->name);
				strcpy(pass,i->pass);
				n_clients = i->n_clients;
				
				strcpy(i->name,b->name);
				strcpy(i->pass,b->pass);
				i->n_clients = b->n_clients;
				
				strcpy(b->name,name);
				strcpy(b->pass,pass);
				b->n_clients =n_clients;
			}
		}
	}
	
}
void deallocate_mem()
{
	free(clnts_gl);
	free(room_gl);
	clnts_gl = NULL;
	room_gl = NULL;
}
