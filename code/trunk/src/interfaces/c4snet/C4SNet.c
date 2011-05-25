/** <!--********************************************************************-->
 *
 * $Id$
 *
 * file C4SNet.c
 *
 * C4SNet is a simple C language interface for S-Net. 
 *
 * 
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "C4SNet.h"
#include "memfun.h"
#include "snettypes.h"
#include "interface_functions.h"
#include "out.h"
#include "base64.h"


#define F_COUNT( c) c->counter[0]
#define T_COUNT( c) c->counter[1]
#define B_COUNT( c) c->counter[2]

#define BUF_SIZE 32

/* ID of the language interface. */
static int interface_id;

/* Container for returning the result. */
struct container {
  struct handle *hnd;
  snet_variant_t *variant;

  int *counter;
  snet_ref_t **fields;
  int *tags;
  int *btags;
};

/* Union containing all C primary types. */
typedef union primary_types {
  unsigned char uc;
  char c;
  unsigned short us;
  short s;
  unsigned int ui;
  int i;
  unsigned long ul;
  long l;
  float f;
  double d;
  long double ld;
  char *str;                  /* Experimental for string */
  void *ptr;
} c4snet_primary_type_t;

/* Data struct to store the actual data. */
struct cdata {
  unsigned int ref_count;     /* reference count for garbage collection. */
  c4snet_vtype_t vtype;       /* Variable type. */
  int size;                   /* Number of elements in the array. */
  c4snet_type_t type;         /* C type of the data. */
  c4snet_primary_type_t data; /* The data. */
};

/* Interface functions that are needed by the runtime environment,
 * but are not given to the box programmer.
 */
static void *C4SNetDataShallowCopy( void *ptr);
static int C4SNetDataSerialize( FILE *file, void *ptr);
static void *C4SNetDataDeserialize(FILE *file);
static int C4SNetDataEncode( FILE *file, void *ptr);
static void *C4SNetDataDecode(FILE *file);

/***************************** Auxiliary functions ****************************/

/* Return size of the data type. */
int C4SNetSizeof(c4snet_data_t *data)
{
  int size;

  switch(data->type) {
  case CTYPE_uchar: 
    size = sizeof(unsigned char);
    break;
  case CTYPE_char: 
    size = sizeof(signed char);
    break;
  case CTYPE_ushort: 
    size = sizeof(unsigned short);
    break;
  case CTYPE_short: 
    size = sizeof(signed short);
    break;
  case CTYPE_uint: 
    size = sizeof(unsigned int);
    break;
  case CTYPE_int: 
    size = sizeof(signed int);
    break;
  case CTYPE_ulong:
    size = sizeof(unsigned long);
    break;
  case CTYPE_long:
    size = sizeof(signed long);
    break;
  case CTYPE_float: 
    size = sizeof(float);
    break;
  case CTYPE_double: 
    size = sizeof(double);
    break;
  case CTYPE_ldouble: 
    size = sizeof(long double);
    break;
  case CTYPE_string:
    size = sizeof(char *);
    break;
  case CTYPE_unknown:
  default:
    size = 0;
    break;
  }

  return size;
}

/***************************** Common functions ****************************/

/* Language interface initialization function. */
void C4SNetInit( int id)
{
  interface_id = id;
  SNetInterfaceRegister( id,
                         &C4SNetDataFree,
                         &C4SNetDataShallowCopy,
                         &C4SNetDataSerialize,
                         &C4SNetDataDeserialize,
                         &C4SNetDataEncode,
                         &C4SNetDataDecode);
}

/* Communicates back the results of the box. */
void C4SNetOut( void *hnd, int variant, ...)
{
  va_list args;
 
  va_start( args, variant);
  SNetOutRawV( (struct handle *)hnd, interface_id, variant, args);
  va_end( args);
}


/****************************** Data functions *****************************/

/* Creates a new c4snet_data_t struct. */
c4snet_data_t *C4SNetDataCreate( c4snet_type_t type, const void *data)
{
  c4snet_data_t *c = NULL;
  int size;

  if(type == CTYPE_unknown || data == NULL) {
    return NULL;
  }

  if((c = (c4snet_data_t *)SNetMemAlloc( sizeof( c4snet_data_t))) == NULL) {
    return NULL;
  }

  //if (type == CTYPE_int) printf("%s, the address: %i\n", __func__, data);
  
  c->vtype = VTYPE_simple;
  c->size = -1;
  c->type = type;
  c->ref_count = 1;

  size = C4SNetSizeof(c);
  
  if (type == CTYPE_string) {
    c->data.str = strdup((char *)data);
  }
  else {
    memcpy((void *)&c->data, data, size);
  }

  return( c);
}

c4snet_data_t *C4SNetDataCreateArray( c4snet_type_t type, int size, void *data)
{
  c4snet_data_t *c = NULL;

  if(type == CTYPE_unknown || data == NULL) {
    return NULL;
  }

  if((c = (c4snet_data_t *)SNetMemAlloc( sizeof( c4snet_data_t))) == NULL) {
    return NULL;
  }

  c->vtype = VTYPE_array;
  c->size = size;
  c->type = type;
  c->ref_count = 1;

  c->data.ptr = data;

  return( c);
}

/* Increments c4snet_data_t struct's reference count by one.
 * This copy function should be used inside S-Net runtime
 * to avoid needles copying.
 */
static void *C4SNetDataShallowCopy( void *ptr)
{
  c4snet_data_t *temp = (c4snet_data_t *)ptr;

  if(ptr != NULL) {
    temp->ref_count++;
  }

  return ptr;
}

/* Copies c4snet_data_t struct. 
 * This copy function is available for box code writers.
 *
 */
void *C4SNetDataCopy( void *ptr)
{
  c4snet_data_t *temp = (c4snet_data_t *)ptr;
  c4snet_data_t *c = NULL;
  int size;

  if((c = (c4snet_data_t *)SNetMemAlloc( sizeof( c4snet_data_t))) == NULL) {
    return NULL;
  }

  c->vtype = temp->vtype;
  c->size = temp->size;
  c->type = temp->type;
  c->ref_count = 1;

  size = C4SNetSizeof(c);

  if(c->vtype == VTYPE_array) {
    if((c->data.ptr = SNetMemAlloc(size * temp->size)) == NULL) {
      SNetMemFree(c);
      return NULL;
    }
    memcpy(c->data.ptr, temp->data.ptr, size * temp->size);
  } 
  else {
    if (c->type == CTYPE_string) {
        c->data.str = strdup(temp->data.str);
    }
    else {
        memcpy((void *)&c->data, (void *)&(temp->data), size);
    }
  }

  return c;
}

/* Frees the memory allocated for c4snet_data_t struct. */
void C4SNetDataFree( void *ptr)
{
  c4snet_data_t *temp = (c4snet_data_t *)ptr;

  if(temp != NULL) { 
    temp->ref_count--;
    
    if(temp->ref_count == 0) {
      if(temp->vtype == VTYPE_array) {
	SNetMemFree( temp->data.ptr);
      }
      SNetMemFree( ptr);
    }
  }
}

/* Returns the actual data. */
void *C4SNetDataGetData( c4snet_data_t *ptr)
{
  if(ptr != NULL) {
    if(ptr->type == CTYPE_string) {
        return ptr->data.str;
    }

    if(ptr->vtype == VTYPE_array) {
      return ptr->data.ptr;
    } else {
      return (void *)&ptr->data;
    }
  } 

  return ptr;
}

int C4SNetDataGetArraySize( c4snet_data_t *ptr)
{
  if(ptr != NULL && ptr->vtype == VTYPE_array) {
    return ptr->size;
  }

  return -1;
}

static int C4SNetDataSerializeDataPart( FILE *file, c4snet_type_t type, void *data)
{
  int ret = 0;

  switch(type) {
  case CTYPE_uchar: 
    
    /* '&' and '<' must be encoded to '&amp;' and 'lt'; 
     * as they are not legal characters in  XML character data!
     */
    
    if(*(char *)(data) == '<') {
      ret += fprintf(file, "&lt;");
    }
    if(*(char *)(data) == '&') {
      ret += fprintf(file, "&amp;");
    }
    ret += fprintf(file, "%c", *(char *)(data));
    break;
  case CTYPE_char: 
    
    /* '&' and '<' must be encoded to '&amp;' and 'lt'; 
     * as they are not legal characters in  XML character data!
     */
    
    if(*(char *)(data) == '<') {
      ret += fprintf(file, "&lt;");
      }
    if(*(char *)(data) == '&') {
      ret += fprintf(file, "&amp;");
    }   
    ret += fprintf(file, "%c", *(char *)(data));
    break;
  case CTYPE_string:
    ret += fprintf(file, "\"%s\"", ((c4snet_primary_type_t *)data)->str );
    break;
  case CTYPE_ushort: 
    ret += fprintf(file, "%hu", *(unsigned short *)data);
    break;
  case CTYPE_short: 
    ret += fprintf(file, "%hd", *(short *)data);
    break;
  case CTYPE_uint: 
    ret += fprintf(file, "%u", *(unsigned int *)data);
    break;
  case CTYPE_int: 
    ret += fprintf(file, "%d", *(int *)data);
    break;
  case CTYPE_ulong:
    ret += fprintf(file, "%lu", *(unsigned long *)data);
    break;
  case CTYPE_long:
    ret += fprintf(file, "%ld", *(long *)data);
    break;
  case CTYPE_float: 
    ret += fprintf(file, "%.32e", *(double *)data);	
    break;
  case CTYPE_double: 
    ret += fprintf(file, "%.32le", *(double *)data);
    break;
  case CTYPE_ldouble: 
    ret += fprintf(file, "%.32le", *(/* long */ double *)data);
      break;
  default:
    ret += 0;
    break;
  }
  return ret;
}

/* Serializes data to a file using textual representation. */
static int C4SNetDataSerialize( FILE *file, void *ptr)
{
  c4snet_data_t *temp = (c4snet_data_t *)ptr;
  int i;
  int ret = 0;
  int size;

  if(temp == NULL && temp->type != CTYPE_unknown) {
    return 0;
  }
  ret += fprintf(file, "(");

  switch(temp->type) {
  case CTYPE_uchar: 
    ret += fprintf(file, "unsigned char");
    break;
  case CTYPE_char: 
    ret += fprintf(file, "char");
    break;
  case CTYPE_ushort: 
    ret += fprintf(file, "unsigned short");
    break;
  case CTYPE_short: 
    ret += fprintf(file, "short");
    break;
  case CTYPE_uint: 
    ret += fprintf(file, "unsigned int");
    break;
  case CTYPE_int: 
    ret += fprintf(file, "int");
    break;
  case CTYPE_ulong:
    ret += fprintf(file, "unsigned long");
    break;
  case CTYPE_long:
    ret += fprintf(file, "long");
    break;
  case CTYPE_float: 
    ret += fprintf(file, "float");	
    break;
  case CTYPE_double: 
    ret += fprintf(file, "double");
    break;
  case CTYPE_ldouble: 
    ret += fprintf(file, "long double");
    break;
  case CTYPE_string:
    ret += fprintf(file, "string");
    break;
  default:
    ret += 0;
    break;
  }


  if(temp->vtype == VTYPE_array) {
    ret += fprintf(file, "[%d]", temp->size);
  } 

  ret += fprintf(file, ")");  
  
  if(temp->vtype == VTYPE_array) {

    size = C4SNetSizeof(temp);

    for(i = 0; i < temp->size; i++) {
      // TODO This conversion `(c4snet_primary_type_t *)((size_t)temp->data.ptr + i * size)' 
      // should be chekced properly, this is done for compiling this file with g++
      ret += C4SNetDataSerializeDataPart(file, temp->type, (c4snet_primary_type_t *)((size_t)temp->data.ptr + i * size));

      if(i < temp->size - 1) {
	ret += fprintf(file, ",");
      }
    }
  } else {
    ret += C4SNetDataSerializeDataPart(file, temp->type, &temp->data);
  }

  return ret;
}

static int C4SNetDataDeserializeTypePart(const char *buf, int size, c4snet_vtype_t *vtype, c4snet_type_t *type) 
{
  int count = 0;
  char *c;
  int n;

  c = strchr(buf, '[');

  if(c == NULL) {
    count = -1;
    *vtype = VTYPE_simple;
  } else {
    count = (int)strtol(c + 1, NULL, 0);
    *vtype = VTYPE_array;
    n = c - buf;
  }

  c = strchr(buf, ')');

  if(c == NULL) {
    *vtype = VTYPE_unknown;
    *type = CTYPE_unknown;
    return 0;
  } 

  if(*vtype == VTYPE_simple) {
    n = c - buf;   
  }

  if(strncmp(buf, "(unsigned char", n) == 0) {
    *type = CTYPE_uchar;
  }
  else if(strncmp(buf, "(char", n) == 0) {
    *type = CTYPE_char;
  }
  else if(strncmp(buf, "(unsigned short", n) == 0) {
    *type = CTYPE_ushort;
  }
  else if(strncmp(buf, "(short", n) == 0) {
    *type = CTYPE_short;
  }
  else if(strncmp(buf, "(unsigned int", n) == 0) {
    *type = CTYPE_uint;
  }
  else if(strncmp(buf, "(int", n) == 0) {
    *type = CTYPE_int;
  }
  else if(strncmp(buf, "(unsigned long", n) == 0) {
    *type = CTYPE_ulong;
  }
  else if(strncmp(buf, "(long", n) == 0) {
    *type = CTYPE_long; 
  }
  else if(strncmp(buf, "(float", n) == 0) {
    *type = CTYPE_float;
  }
  else if(strncmp(buf, "(double", n) == 0) {
    *type = CTYPE_double;
  } 
  else if(strncmp(buf, "(long double", n) == 0) {
    *type = CTYPE_ldouble; 
  } 
  else if(strncmp(buf, "(string", n) == 0) {
    *type = CTYPE_string;
  }  else {
    *type = CTYPE_unknown;
  }

  return count;
}

static int C4SNetDataDeserializeDataPart( FILE *file, c4snet_type_t type, c4snet_primary_type_t *data/*void *data*/)
{
  int ret;
  char buf[BUF_SIZE];
  char c;
  int j;

  switch(type) {
  case CTYPE_uchar: 
    
    /* '&' and '<' must be encoded to '&amp;' and 'lt'; 
     * as they are not legal characters in  XML character data!
     */

    if((ret = fscanf(file, "%c", &data->c)) == 1) {

      /* '&' and '<' have been encoded to '&amp;' and 'lt'; 
       * as they are not legal characters in  XML character data!
       */

      if(data->c == '&') {
	j = 0;
	c = '\0';
	while((c = fgetc(file)) != EOF) {
	  buf[j++] = c;
	  if(c == '<') {
	    ungetc('<', file);
	    return 0;
	  }
	  if(c == ';') {
	    break;
	  }

	  if(j >= BUF_SIZE) {
	    return 0;
	  }
	} 

	buf[j] = '\0';

	if(strcmp(buf, "amp;") == 0) {
	  data->c = '&';
	}
	else if(strcmp(buf, "lt;") == 0) {
	  data->c = '<';
	}else{
	  return 0;
	}
      } 
      else if(data->c == '<') {
	ungetc('<', file);
	return 0;
      }
      data->uc = (unsigned char)data->c;
    }

    return ret;

    break;
  case CTYPE_char: 
    if((ret = fscanf(file, "%c", &data->c)) == 1) {
      
      /* '&' and '<' have been encoded to '&amp;' and 'lt'; 
       * as they are not legal characters in  XML character data!
       */

      if(data->c == '&') {
	j = 0;
	c = '\0';
	while((c = fgetc(file)) != EOF) {
	  buf[j++] = c;
	  if(c == '<') {
	    ungetc('<', file);
	    return 0;;
	  }
	  if(c == ';') {
	    break;
	  }
	  if(j >= BUF_SIZE) {
	    return 0;
	  }
	} 

	buf[j] = '\0';
	printf("data: %s\n", buf);
	
	if(strcmp(buf, "amp;") == 0) {
	  data->c = '&';
	} 
	else if(strcmp(buf, "lt;") == 0) {
	  data->c = '<';
	}else{
	  return 0;
	}
      }
      else if(*(char *)data == '<') {
	ungetc('<', file);
	return 0;
      }
    }

    return ret;
    break;
  
  case CTYPE_string:
  {
#   define SIZE 3
    char * buffer=NULL;
    int size=0, cur=0;
    char symbol;
    fscanf(file, "%c", &symbol);

    if (symbol != '\"') return 0;
    while (symbol != 0) {
        fscanf(file, "%c", &symbol);
        if (symbol == '\"') symbol = 0;
        if (cur == size) {
            size += SIZE;
            if (buffer == NULL) {
                buffer = (char *) malloc(size * sizeof(char));
            }
            else
                buffer = (char *) realloc(buffer, size * sizeof(char));
        }
        buffer[cur++] = symbol;
    }
    data->str = buffer;
    //printf("C4SNet interface string [%s]\n", data->str);
    return 1;
  }
  break;
  case CTYPE_ushort: 
    return fscanf(file, "%hu", &data->us);
    break;
  case CTYPE_short: 
    return fscanf(file, "%hd", &data->s);
    break;
  case CTYPE_uint: 
    return fscanf(file, "%u",  &data->ui);
    break;
  case CTYPE_int: 
    return fscanf(file, "%d", &data->i);  
    break;
  case CTYPE_ulong:
    return fscanf(file, "%lu", &data->ul);
    break;
  case CTYPE_long:
    return fscanf(file, "%ld", &data->l);
    break;
  case CTYPE_float: 
    return fscanf(file, "%f",  &data->f);
    break;
  case CTYPE_double: 
    return fscanf(file, "%lf", &data->d);
    break;
  case CTYPE_ldouble: 
    return fscanf(file, "%Lf", &data->ld);
    break;
  default:
    break;
  }
  return 0;
}

/* Deserializes textual data from a file. */
static void *C4SNetDataDeserialize(FILE *file)
{
  c4snet_data_t *temp = NULL;
  char buf[BUF_SIZE];
  int j;
  char c;
  int size;
  int i;

  j = 0;
  c = '\0';
  while((c = fgetc(file)) != EOF) {
    buf[j++] = c;
    if(c == '<') {
      ungetc('<', file);
      return NULL;
    }
    if(c == ')') {
      break;
    }
    if(j >= BUF_SIZE) {
      return NULL;
    }
  } 

  buf[j] = '\0';

  temp = (c4snet_data_t *)SNetMemAlloc( sizeof( c4snet_data_t));

  temp->size = C4SNetDataDeserializeTypePart(buf, j - 1, &temp->vtype, &temp->type); 

  temp->ref_count = 1;


  if(temp->vtype == VTYPE_array) {

    size = C4SNetSizeof(temp);

    temp->data.ptr = SNetMemAlloc(size * temp->size);

    for(i = 0; i < temp->size; i++) {
      
      if(i > 0 && (c = fgetc(file)) != ',') {
	SNetMemFree(temp->data.ptr);
	SNetMemFree(temp);
	return NULL;
      }
      
      // TODO This conversion `(c4snet_primary_type_t *)((size_t)temp->data.ptr + i * size)' 
      // should be chekced properly, this is done for compiling this file with g++
      if(C4SNetDataDeserializeDataPart(file, temp->type, (c4snet_primary_type_t *)((size_t)temp->data.ptr + i * size)) == 0) {
	SNetMemFree(temp->data.ptr);
	SNetMemFree(temp);
	return NULL;
      }
    }
  } 
  else 
  {
    //c4snet_primary_type_t tmp;

    if(C4SNetDataDeserializeDataPart(file, temp->type, &temp->data) == 0) {
       SNetMemFree(temp);
      return NULL;
    }
    //if (temp->type == CTYPE_string){
    //    printf("%s, %s\n", __func__, temp->data.str);
    //}
    //tmp = (c4snet_primary_type_t *) &tmp1;
    //temp->data = tmp;
  }

  return temp;
}

/* Binary encodes data to a file. */
static int C4SNetDataEncode( FILE *file, void *ptr)
{
  c4snet_data_t *temp = (c4snet_data_t *)ptr;
  int i = 0;

  if(temp == NULL) {
    return 0;
  }

  i = Base64encodeDataType(file, (int)temp->vtype);

  i += Base64encode(file, (unsigned char *)&temp->size, sizeof(int));

  i = Base64encodeDataType(file, (int)temp->type);

  if(temp->vtype == VTYPE_array) {
    i += Base64encode(file, (unsigned char *)temp->data.ptr, C4SNetSizeof(temp) * temp->size);
  } else {
    i += Base64encode(file, (unsigned char *)&temp->data, C4SNetSizeof(temp));
  }

  return i + 1;
}

/* Decodes binary data from a file. */
static void *C4SNetDataDecode(FILE *file)
{
  c4snet_data_t *c = NULL;
  int i = 0;
  int size;

  c = (c4snet_data_t *)SNetMemAlloc( sizeof( c4snet_data_t));

  i = Base64decodeDataType(file, (int *)&c->vtype);

  i += Base64decode(file, (unsigned char *)&c->size, sizeof(int));

  i = Base64decodeDataType(file, (int *)&c->type);

  c->ref_count = 1;

  size = C4SNetSizeof(c);
  
  if(c->vtype == VTYPE_array) {
    c->data.ptr = SNetMemAlloc(size * c->size);
    i += Base64decode(file, (unsigned char *)c->data.ptr, size * c->size);
  } else {
    i += Base64decode(file, (unsigned char *)&c->data, size);
  }

  return c;
}



/**************************** Container functions ***************************/

/* Creates a container that can be used to return values. */
c4snet_container_t *C4SNetContainerCreate( void *hnd, int variant)
{
  int i;
  c4snet_container_t *c;

  if(hnd == NULL) {
    return 0;
  }
  //FIXME: diagnostics for non-existent variant

  c = (c4snet_container_t *)SNetMemAlloc( sizeof( c4snet_container_t));
  //variant - 1: Lists are zero indexed, but existing S-Net code defines
  //variants as 1-indexed. Go backwards compatibility!
  c->variant = SNetVariantListGet( SNetHndGetVariants((struct handle *) hnd), variant - 1);

  c->counter = SNetMemAlloc( 3 * sizeof( int));

  c->fields = SNetMemAlloc( SNetVariantNumFields( c->variant) * sizeof(snet_ref_t*));
  c->tags = SNetMemAlloc( SNetVariantNumTags( c->variant) * sizeof( int));
  c->btags = SNetMemAlloc( SNetVariantNumBTags( c->variant) * sizeof( int));
  c->hnd = hnd;

  for( i=0; i<3; i++) {
    c->counter[i] = 0;
  }

  return c;
}


/* Adds field into a container. */
c4snet_container_t *C4SNetContainerSetField(c4snet_container_t *c, void *ptr)
{
  c->fields[ F_COUNT( c)] = ptr;
  F_COUNT( c) += 1;

  return( c);
}

/* Adds tag into a container. */
c4snet_container_t *C4SNetContainerSetTag(c4snet_container_t *c, int value)
{
  c->tags[ T_COUNT( c)] = value;
  T_COUNT( c) += 1;

  return( c);
}

/* Adds binding tag into a container. */
c4snet_container_t *C4SNetContainerSetBTag(c4snet_container_t *c, int value)
{
  c->btags[ B_COUNT( c)] = value;
  B_COUNT( c) += 1;

  return( c);
}

/* Communicates back the results of the box stored in a container. */
void C4SNetContainerOut(c4snet_container_t *c)
{
  SNetOutRawArray( c->hnd, interface_id, c->variant, c->fields, c->tags, c->btags);

  SNetMemFree(c->counter);
  SNetMemFree(c);
}