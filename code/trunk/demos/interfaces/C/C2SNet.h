#ifndef _C2SNet_h_
#define _C2SNet_h_

typedef struct container c2snet_container_t;

typedef struct cdata {
 void *data;
 void (*freefun)( void*);
 void* (*copyfun)( void*);
} C_Data;


void C2SNet_init( int id);

void C2SNet_out( c2snet_container_t *c);
void C2SNet_outRaw( void *hnd, int variant, ...);

void C2SNet_free( void *ptr);
void *C2SNet_copy( void *ptr);

/* ************************************************************************* */

/* C_Data */

C_Data *C2SNet_cdataCreate( void *data, 
		     	    void (*freefun)( void*),
		     	    void* (*copyfun)( void*));

void *C2SNet_cdataGetData( C_Data *c);
void *C2SNet_cdataGetCopyFun( C_Data *c);
void *C2SNet_cdataGetFreeFun( C_Data *c);

/* ************************************************************************* */

c2snet_container_t *C2SNet_containerCreate( void *hnd, int var_num);
c2snet_container_t *C2SNet_containerSetField( c2snet_container_t *c, void *ptr);
c2snet_container_t *C2SNet_containerSetTag( c2snet_container_t *c, int val);
c2snet_container_t *C2SNet_containerSetBTag( c2snet_container_t *c, int val);
#endif 