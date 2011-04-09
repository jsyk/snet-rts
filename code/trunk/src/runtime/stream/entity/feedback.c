#include <assert.h>

#include "snetentities.h"

#include "typeencode.h"
#include "expression.h"
#include "memfun.h"
#include "queue.h"

#include "threading.h"
#include "distribution.h"


/* Helper function for the feedback-dispatcher
 * - copied from star.c
 */
static bool MatchesBackPattern( snet_record_t *rec,
    snet_typeencoding_t *back_patterns, snet_expr_list_t *guards)
{
  int i;
  bool is_match;
  snet_variantencoding_t *pat;

  for( i=0; i<SNetTencGetNumVariants( back_patterns); i++) {
    is_match = true;

    if( SNetEevaluateBool( SNetEgetExpr( guards, i), rec)) {
      pat = SNetTencGetVariant( back_patterns, i+1);
      is_match = SNetRecPatternMatches(pat, rec);
    }
    else {
      is_match = false;
    }
    if( is_match) {
      break;
    }
  }
  return( is_match);
}





/******************************************************************************
 * Feedback collector
 *****************************************************************************/

typedef struct {
  snet_stream_t *in, *fbi, *out;
} fbcoll_arg_t;

enum fbcoll_mode {
  FBCOLL_IN,
  FBCOLL_FB1,
  FBCOLL_FB0
};


struct fbcoll_state {
    snet_stream_desc_t *instream;
    snet_stream_desc_t *outstream;
    snet_stream_desc_t *backstream;
    bool terminate;
    enum fbcoll_mode mode;
};

/* helper functions to handle mode the feedback collector is in */


static void FbCollReadIn(struct fbcoll_state *state)
{
  snet_record_t *rec;

  assert( false == state->terminate );

  /* read from input stream */
  rec = SNetStreamRead( state->instream);

  switch( SNetRecGetDescriptor( rec)) {

    case REC_data:
      /* relay data record */
      SNetStreamWrite( state->outstream, rec);
      /* append a sort record */
      SNetStreamWrite(
          state->outstream,
          SNetRecCreate( REC_sort_end, 0, 1 ) /*type, lvl, num*/
          );
      /* mode switch to FB1 */
      state->mode = FBCOLL_FB1;
      break;

    case REC_sort_end:
      /* increase the level and forward */
      SNetRecSetLevel( rec, SNetRecGetLevel(rec)+1);
      SNetStreamWrite( state->outstream, rec);
      break;

    case REC_terminate:
      state->terminate = true;
      SNetStreamWrite( state->outstream, rec);
      /* note that no sort record has to be appended */
      break;

    case REC_sync:
      SNetStreamReplace( state->instream, SNetRecGetStream( rec));
      SNetRecDestroy( rec);
      break;

    case REC_collect:
    default:
      assert(0);
      /* if ignoring, at least destroy ... */
      SNetRecDestroy( rec);
      break;
  }
}


static void FbCollReadFbi(struct fbcoll_state *state)
{
  snet_record_t *rec;

  assert( false == state->terminate );

  /* read from feedback stream */
  rec = SNetStreamRead( state->backstream);

  switch( SNetRecGetDescriptor( rec)) {

    case REC_data:
      /* relay data record */
      SNetStreamWrite( state->outstream, rec);
      /* mode switch to FB0 (there is a next iteration) */
      state->mode = FBCOLL_FB0;
      break;

    case REC_sort_end:
      assert( 0 == SNetRecGetLevel(rec) );
      switch(state->mode) {
        case FBCOLL_FB0:
          state->mode = FBCOLL_FB1;
          /* increase counter (non-functional) */
          SNetRecSetNum( rec, SNetRecGetNum(rec)+1);
          SNetStreamWrite( state->outstream, rec);
          break;
        case FBCOLL_FB1:
          state->mode = FBCOLL_IN;
          /* kill the sort record */
          SNetRecDestroy( rec);
          break;
        default: assert(0);
      }
      break;

    case REC_terminate:
    case REC_sync:
    case REC_collect:
    default:
      assert(0);
      /* if ignoring, at least destroy ... */
      SNetRecDestroy( rec);
      break;
  }

}


/**
 * The feedback collector, the entry point of the
 * feedback combinator loop
 */
static void FeedbackCollTask( snet_entity_t *self, void *arg)
{
  fbcoll_arg_t *fbcarg = (fbcoll_arg_t *)arg;
  struct fbcoll_state state;

  /* initialise state */
  state.terminate = false;
  state.mode = FBCOLL_IN;

  state.instream   = SNetStreamOpen( self, fbcarg->in,  'r');
  state.backstream = SNetStreamOpen( self, fbcarg->fbi, 'r');
  state.outstream  = SNetStreamOpen( self, fbcarg->out, 'w');
  SNetMemFree( fbcarg);

  /* MAIN LOOP */
  while( !state.terminate) {

    /* which stream to read from is mode dependent */
    switch(state.mode) {
      case FBCOLL_IN:
        FbCollReadIn(&state);
        break;
      case FBCOLL_FB1:
      case FBCOLL_FB0:
        FbCollReadFbi(&state);
        break;
      default: assert(0); /* should not be reached */
    }

  } /* END OF MAIN LOOP */

  SNetStreamClose(state.instream,   true);
  SNetStreamClose(state.backstream, true);
  SNetStreamClose(state.outstream,  false);

}




/******************************************************************************
 * Feedback dispatcher
 *****************************************************************************/


typedef struct {
  snet_stream_t *in, *out, *fbo;
  snet_typeencoding_t *back_patterns;
  snet_expr_list_t *guards;
} fbdisp_arg_t;


/**
 * The feedback dispatcher, at the end of the
 * feedback combinator loop
 */
static void FeedbackDispTask( snet_entity_t *self, void *arg)
{
  fbdisp_arg_t *fbdarg = (fbdisp_arg_t *)arg;

  snet_stream_desc_t *instream;
  snet_stream_desc_t *outstream;
  snet_stream_desc_t *backstream;
  bool terminate = false;
  snet_record_t *rec;

  instream   = SNetStreamOpen( self, fbdarg->in,  'r');
  outstream  = SNetStreamOpen( self, fbdarg->out, 'w');
  backstream = SNetStreamOpen( self, fbdarg->fbo, 'w');

  /* MAIN LOOP */
  while( !terminate) {

    /* read from input stream */
    rec = SNetStreamRead( instream);

    switch( SNetRecGetDescriptor( rec)) {

      case REC_data:
        /* route data record */
        if( MatchesBackPattern( rec, fbdarg->back_patterns, fbdarg->guards)) {
          /* send rec back into the loop */
          SNetStreamWrite( backstream, rec);
        } else {
          /* send to output */
          SNetStreamWrite( outstream, rec);
        }
        break;

      case REC_sort_end:
        {
          int lvl = SNetRecGetLevel(rec);
          if ( 0 == lvl ) {
            SNetStreamWrite( backstream, rec);
          } else {
            assert( lvl > 0 );
            SNetRecSetLevel( rec, lvl-1);
            SNetStreamWrite( outstream, rec);
          }
        }
        break;

      case REC_terminate:
        terminate = true;
        SNetStreamWrite( outstream, rec);
        /* a terminate record is sent in the backloop for the buffer */
        SNetStreamWrite( backstream, SNetRecCopy( rec));
        break;

      case REC_sync:
        SNetStreamReplace( instream, SNetRecGetStream( rec));
        SNetRecDestroy( rec);
        break;

      case REC_collect:
      default:
        assert(0);
        /* if ignoring, at least destroy ... */
        SNetRecDestroy( rec);
        break;
    }

  } /* END OF MAIN LOOP */

  SNetStreamClose(instream,   true);
  SNetStreamClose(outstream,  false);
  SNetStreamClose(backstream, false);

  SNetDestroyTypeEncoding( fbdarg->back_patterns);
  SNetEdestroyList( fbdarg->guards);
  SNetMemFree( fbdarg);
}


/******************************************************************************
 * Feedback buffer
 *****************************************************************************/

typedef struct{
  snet_stream_t *in, *out;
} fbbuf_arg_t;


/**
 * The feedback buffer, in the back-loop
 */
static void FeedbackBufTask( snet_entity_t *self, void *arg)
{
  fbbuf_arg_t *fbbarg = (fbbuf_arg_t *)arg;

  snet_stream_desc_t *instream;
  snet_stream_desc_t *outstream;
  snet_queue_t *internal_buffer;
  snet_record_t *rec;
  int K = 10;

  instream   = SNetStreamOpen( self, fbbarg->in,  'r');
  outstream  = SNetStreamOpen( self, fbbarg->out, 'w');
  SNetMemFree( fbbarg);

  internal_buffer = SNetQueueCreate();

  /* MAIN LOOP */
  while(1) {
    int n = 0;
    rec = NULL;

    /* STEP 1: read n=min(available,K) records from input stream */

    /* read first record of the actual dispatch */
    if (0 == SNetQueueSize(internal_buffer)) {
      rec = SNetStreamRead(instream);
      /* only in empty mode! */
      if( REC_terminate == SNetRecGetDescriptor( rec)) {
        /* this means, the outstream does not exist anymore! */
        SNetRecDestroy(rec);
        goto feedback_buf_epilogue;
      }
    } else {
      SNetEntityYield(self);
      if ( SNetStreamPeek(instream) != NULL ) {
        rec = SNetStreamRead(instream);
        assert( REC_terminate != SNetRecGetDescriptor( rec) );
      }
    }

    if (rec != NULL) {
      n = 1;
      /* put record into internal buffer */
      (void) SNetQueuePut(internal_buffer, rec);
    }


    while ( n<=K && SNetStreamPeek(instream)!=NULL ) {
      rec = SNetStreamRead(instream);
      /* put record into internal buffer */
      (void) SNetQueuePut(internal_buffer, rec);
      n++;
    }

    /* STEP 2: try to empty the internal buffer */
    rec = SNetQueuePeek(internal_buffer);
    while (rec != NULL) {
      if (0 == SNetStreamTryWrite(outstream, rec)) {
        snet_record_t *rem;
        /* success, also remove from queue */
        rem = SNetQueueGet(internal_buffer);
        assert( rem == rec );
      } else {
        /* there remain elements in the buffer */
        break;
      }
      /* for the next iteration */
      rec = SNetQueuePeek(internal_buffer);
    }

  } /* END OF MAIN LOOP */

feedback_buf_epilogue:

  SNetQueueDestroy(internal_buffer);

  SNetStreamClose(instream,   true);
  SNetStreamClose(outstream,  false);
}



/****************************************************************************/
/* CREATION FUNCTION                                                        */
/****************************************************************************/
snet_stream_t *SNetFeedback( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_typeencoding_t *back_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a
    )
{
  snet_stream_t *output;

  input = SNetRouteUpdate(info, input, location);
  if(location == SNetNodeLocation) {
    snet_stream_t *into_op, *from_op;
    snet_stream_t *back_bufin, *back_bufout;
    fbbuf_arg_t *fbbarg;
    fbcoll_arg_t *fbcarg;
    fbdisp_arg_t *fbdarg;

    /* create streams */
    into_op = SNetStreamCreate(0);
    output  = SNetStreamCreate(0);
    back_bufin  = SNetStreamCreate(0);

//#define FEEDBACK_OMIT_BUFFER


#ifndef FEEDBACK_OMIT_BUFFER
    back_bufout = SNetStreamCreate(0);

    /* create the feedback buffer */
    fbbarg = SNetMemAlloc( sizeof( fbbuf_arg_t));
    fbbarg->in  = back_bufin;
    fbbarg->out = back_bufout;
    SNetEntitySpawn( ENTITY_FBBUF, FeedbackBufTask, (void*)fbbarg );
#else
    back_bufout = back_bufin;
#endif

    /* create the feedback collector */
    fbcarg = SNetMemAlloc( sizeof( fbcoll_arg_t));
    fbcarg->in = input;
    fbcarg->fbi = back_bufout;
    fbcarg->out = into_op;
    SNetEntitySpawn( ENTITY_FBCOLL, FeedbackCollTask, (void*)fbcarg );

    /* create the instance network */
    from_op = box_a(into_op, info, location);

    /* create the feedback dispatcher */
    fbdarg = SNetMemAlloc( sizeof( fbdisp_arg_t));
    fbdarg->in = from_op;
    fbdarg->fbo = back_bufin;
    fbdarg->out = output;
    fbdarg->back_patterns = back_patterns;
    fbdarg->guards = guards;
    SNetEntitySpawn( ENTITY_FBDISP, FeedbackDispTask, (void*)fbdarg );

  } else {
    output = input;
  }

  return( output);


}

