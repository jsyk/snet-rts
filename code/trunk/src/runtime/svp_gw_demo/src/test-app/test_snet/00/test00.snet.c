/**
 * @file test00.c
 *
 * Source code of compiled snet-file for runtime.
 *
 * THIS FILE HAS BEEN GENERATED.
 * DO NOT EDIT THIS FILE.
 * EDIT THE ORIGINAL SNET-SOURCE FILE test00.snet INSTEAD!
 *
 * ALL CHANGES MADE TO THIS FILE WILL BE OVERWRITTEN!
 *
*/

#include "test00.h"
#include "networkinterface.h"

char *snet_test00_labels[] = {
	"E__test00__None",
	"a",
	"b",
	"c",
	"d",
	"e",
	"f",
	"T"};

char *snet_test00_interfaces[] = {};


static void SNet__test00__A(snet_handle_t *hnd)
{
  snet_record_t *rec = NULL;
  void *field_a = NULL;

  rec = SNetHndGetRecord(hnd);

  field_a = SNetRecTakeField(rec, F__test00__a);

  SNetCall__A(hnd, field_a);
}


static void SNet__test00__B(snet_handle_t *hnd)
{
  snet_record_t *rec = NULL;
  void *field_b = NULL;

  rec = SNetHndGetRecord(hnd);

  field_b = SNetRecTakeField(rec, F__test00__b);

  SNetCall__B(hnd, field_b);
}


static void SNet__test00__C(snet_handle_t *hnd)
{
  snet_record_t *rec = NULL;
  void *field_c = NULL;

  rec = SNetHndGetRecord(hnd);

  field_c = SNetRecTakeField(rec, F__test00__c);

  SNetCall__C(hnd, field_c);
}


static void SNet__test00__D(snet_handle_t *hnd)
{
  snet_record_t *rec = NULL;
  void *field_d = NULL;

  rec = SNetHndGetRecord(hnd);

  field_d = SNetRecTakeField(rec, F__test00__d);

  SNetCall__D(hnd, field_d);
}


static void SNet__test00__E(snet_handle_t *hnd)
{
  snet_record_t *rec = NULL;
  void *field_e = NULL;

  rec = SNetHndGetRecord(hnd);

  field_e = SNetRecTakeField(rec, F__test00__e);

  SNetCall__E(hnd, field_e);
}

static snet_tl_stream_t *SNet__test00___SL___SL(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;
  snet_typeencoding_t *out_type = NULL;
  snet_box_sign_t *out_sign = NULL;

  out_type = SNetTencTypeEncode(2, 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__test00__b), 
                SNetTencCreateVector(0), 
                SNetTencCreateVector(0)), 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__test00__b), 
                SNetTencCreateVector(1, T__test00__T), 
                SNetTencCreateVector(0)));


  out_sign = SNetTencBoxSignEncode( out_type, 
              SNetTencCreateVector(1, field), 
              SNetTencCreateVector(2, field, tag));


  out_buf = SNetBox(in_buf, 
              &SNet__test00__A, 
              out_sign);

  return (out_buf);
}

static snet_tl_stream_t *SNet__test00___SL___SR___ST___SL(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;
  snet_typeencoding_t *out_type = NULL;
  snet_box_sign_t *out_sign = NULL;

  out_type = SNetTencTypeEncode(2, 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__test00__c), 
                SNetTencCreateVector(0), 
                SNetTencCreateVector(0)), 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__test00__d), 
                SNetTencCreateVector(0), 
                SNetTencCreateVector(0)));


  out_sign = SNetTencBoxSignEncode( out_type, 
              SNetTencCreateVector(1, field), 
              SNetTencCreateVector(1, field));


  out_buf = SNetBox(in_buf, 
              &SNet__test00__B, 
              out_sign);

  return (out_buf);
}

static snet_tl_stream_t *SNet__test00___SL___SR___ST___SR___PL(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;
  snet_typeencoding_t *out_type = NULL;
  snet_box_sign_t *out_sign = NULL;

  out_type = SNetTencTypeEncode(3, 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__test00__b), 
                SNetTencCreateVector(0), 
                SNetTencCreateVector(0)), 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__test00__b), 
                SNetTencCreateVector(1, T__test00__T), 
                SNetTencCreateVector(0)), 
              SNetTencVariantEncode(
                SNetTencCreateVector(2, F__test00__b, F__test00__e), 
                SNetTencCreateVector(1, T__test00__T), 
                SNetTencCreateVector(0)));


  out_sign = SNetTencBoxSignEncode( out_type, 
              SNetTencCreateVector(1, field), 
              SNetTencCreateVector(2, field, tag), 
              SNetTencCreateVector(3, field, field, tag));


  out_buf = SNetBox(in_buf, 
              &SNet__test00__C, 
              out_sign);

  return (out_buf);
}

static snet_tl_stream_t *SNet__test00___SL___SR___ST___SR___PR(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;
  snet_typeencoding_t *out_type = NULL;
  snet_box_sign_t *out_sign = NULL;

  out_type = SNetTencTypeEncode(3, 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__test00__b), 
                SNetTencCreateVector(0), 
                SNetTencCreateVector(0)), 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__test00__b), 
                SNetTencCreateVector(1, T__test00__T), 
                SNetTencCreateVector(0)), 
              SNetTencVariantEncode(
                SNetTencCreateVector(2, F__test00__b, F__test00__e), 
                SNetTencCreateVector(1, T__test00__T), 
                SNetTencCreateVector(0)));


  out_sign = SNetTencBoxSignEncode( out_type, 
              SNetTencCreateVector(1, field), 
              SNetTencCreateVector(2, field, tag), 
              SNetTencCreateVector(3, field, field, tag));


  out_buf = SNetBox(in_buf, 
              &SNet__test00__D, 
              out_sign);

  return (out_buf);
}

static snet_tl_stream_t *SNet__test00___SL___SR___ST___SR(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;

  out_buf = SNetParallelDet(in_buf, 
              SNetTencCreateTypeEncodingList(2, 
                SNetTencTypeEncode(1, 
                  SNetTencVariantEncode(
                    SNetTencCreateVector(1, F__test00__c), 
                    SNetTencCreateVector(0), 
                    SNetTencCreateVector(0))), 
                SNetTencTypeEncode(1, 
                  SNetTencVariantEncode(
                    SNetTencCreateVector(1, F__test00__d), 
                    SNetTencCreateVector(0), 
                    SNetTencCreateVector(0)))), 
              &SNet__test00___SL___SR___ST___SR___PL, 
              &SNet__test00___SL___SR___ST___SR___PR);

  return (out_buf);
}

static snet_tl_stream_t *SNet__test00___SL___SR___ST(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;

  out_buf = SNetSerial(in_buf, 
              &SNet__test00___SL___SR___ST___SL, 
              &SNet__test00___SL___SR___ST___SR);

  return (out_buf);
}

static snet_tl_stream_t *SNet____STAR_INCARNATE_test00___SL___SR(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;

  out_buf = SNetStarIncarnate(in_buf, 
              SNetTencTypeEncode(1, 
                SNetTencVariantEncode(
                  SNetTencCreateVector(1, F__test00__e), 
                  SNetTencCreateVector(1, T__test00__T), 
                  SNetTencCreateVector(0))), 
              SNetEcreateList(1, 
                SNetElt( 
                  SNetEtag( T__test00__T), 
                  SNetEconsti( 5))), 
              &SNet__test00___SL___SR___ST, 
              &SNet____STAR_INCARNATE_test00___SL___SR);

  return (out_buf);
}

static snet_tl_stream_t *SNet__test00___SL___SR(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;

  out_buf = SNetStar(in_buf, 
              SNetTencTypeEncode(1, 
                SNetTencVariantEncode(
                  SNetTencCreateVector(1, F__test00__e), 
                  SNetTencCreateVector(1, T__test00__T), 
                  SNetTencCreateVector(0))), 
              SNetEcreateList(1, 
                SNetElt( 
                  SNetEtag( T__test00__T), 
                  SNetEconsti( 5))), 
              &SNet__test00___SL___SR___ST, 
              &SNet____STAR_INCARNATE_test00___SL___SR);

  return (out_buf);
}

static snet_tl_stream_t *SNet__test00___SL(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;

  out_buf = SNetSerial(in_buf, 
              &SNet__test00___SL___SL, 
              &SNet__test00___SL___SR);

  return (out_buf);
}

static snet_tl_stream_t *SNet__test00___SR___IS(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;
  snet_typeencoding_t *out_type = NULL;
  snet_box_sign_t *out_sign = NULL;

  out_type = SNetTencTypeEncode(2, 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__test00__e), 
                SNetTencCreateVector(0), 
                SNetTencCreateVector(0)), 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__test00__f), 
                SNetTencCreateVector(0), 
                SNetTencCreateVector(0)));


  out_sign = SNetTencBoxSignEncode( out_type, 
              SNetTencCreateVector(1, field), 
              SNetTencCreateVector(1, field));


  out_buf = SNetBox(in_buf, 
              &SNet__test00__E, 
              out_sign);

  return (out_buf);
}

static snet_tl_stream_t *SNet__test00___SR(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;

  out_buf = SNetSplit(in_buf, 
              &SNet__test00___SR___IS, T__test00__T, T__test00__T);

  return (out_buf);
}

snet_tl_stream_t *SNet__test00___test00(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;

  out_buf = SNetSerial(in_buf, 
              &SNet__test00___SL, 
              &SNet__test00___SR);

  return (out_buf);
}

int SNetMain__test00(int argc, char* argv[])
{
  int ret = 0;

  SNetGlobalInitialise();

  ret = SNetInRun(argc, argv,
              snet_test00_labels,
              SNET__test00__NUMBER_OF_LABELS,
              snet_test00_interfaces,
              SNET__test00__NUMBER_OF_INTERFACES,
              SNet__test00___test00);

  SNetGlobalDestroy();

  return( ret);
}

