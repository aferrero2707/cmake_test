#include<iostream>
#include <stdio.h>
#include <vips/vips.h>
#include <stdlib.h>

typedef float MY_PX_TYPE;

#define _(a) (a)

#include "imgtree.cc"

#include "paddedop.cc"
#include "slowop.cc"

#define RADIUS 200
const size_t W=9000-100;
const size_t H=W;
const int iterations = 5;

MY_PX_TYPE* ibuf;
MY_PX_TYPE* obuf;



#if defined(__MINGW32__) || defined(__MINGW64__)
#define realpath(N,R) _fullpath((R),(N),_MAX_PATH)
#endif



void fill_pattern(MY_PX_TYPE* ibuf, size_t W, size_t H)
{
  for(int row = 0; row < H; row++) {
    MY_PX_TYPE* pi = ibuf + row*W;
    for(int col = 0; col < W; col++) {
      int row2 = row/100;
      int col2 = col/100;
      int id1 = row2%2;
      int id2 = col2%2;
      MY_PX_TYPE result = 0;
      if(id1>0 && id2==0) result = 255;
      if(id2>0 && id1==0) result = 255;
      pi[col] = result;
    }
  }
}


void vips_run1(int padding, bool do_caching, bool cache_blur)
{
  const int TS = 128;
  GTimer* timer = g_timer_new();

  VipsImage* in = vips_image_new_from_memory( ibuf, W*H*sizeof(MY_PX_TYPE), W, H, 1, VIPS_FORMAT_FLOAT );

  VipsImage* cached1 = in;
  if( false && do_caching ) {
    if( vips_tilecache( in, &cached1,
        "access", VIPS_ACCESS_RANDOM,
        //"access", VIPS_ACCESS_SEQUENTIAL,
        "threaded", TRUE,
        "tile-width", TS,
        "tile-height", TS,
        "max-tiles", 3 * in->Xsize / TS,
        NULL ) ) {
      std::cout << "cannot cache" << std::endl;
      return;
    }
    VIPS_UNREF(in);
  }

  VipsImage* slow;
  vips_slowop(in, &slow, 2.0, NULL);
  //if( vips_gaussblur(cached1, &slow, 10, "precision", VIPS_PRECISION_FLOAT, NULL) ) return;
  VIPS_UNREF(in);

  VipsImage* cached = slow;
  if( do_caching ) {
    if( vips_tilecache( slow, &cached,
        "access", VIPS_ACCESS_RANDOM,
        //"access", VIPS_ACCESS_SEQUENTIAL,
        "threaded", TRUE,
        "tile-width", TS,
        "tile-height", TS,
        "max-tiles", 3 * slow->Xsize / TS,
        NULL ) ) {
      std::cout << "cannot cache" << std::endl;
      return;
    }
    VIPS_UNREF(slow);
/*
    if( vips_tilecache( cached, &cached2,
        "access", VIPS_ACCESS_RANDOM,
        "threaded", TRUE,
        "tile-width", TS,
        "tile-height", TS,
        "max-tiles", 3 * cached->Xsize / TS,
        NULL ) ) {
      std::cout << "cannot cache" << std::endl;
      return;
    }
    VIPS_UNREF(cached);
*/
  }


  VipsImage* padded;
  vips_paddedop(cached, &padded, padding, NULL);
  VIPS_UNREF(cached);

  VipsImage* cached2 = padded;
  if( cache_blur ) {
  if( vips_tilecache( padded, &cached2,
      "access", VIPS_ACCESS_RANDOM,
      //"access", VIPS_ACCESS_SEQUENTIAL,
      "threaded", TRUE,
      "tile-width", TS,
      "tile-height", TS,
      "max-tiles", 3 * padded->Xsize / TS,
      NULL ) ) {
    std::cout << "cannot cache" << std::endl;
    return;
  }
  VIPS_UNREF(padded);
  }

  VipsImage* blurred;
  if( vips_gaussblur(cached2, &blurred, 1, "precision", VIPS_PRECISION_FLOAT, NULL) ) return;
  VIPS_UNREF(cached2);

  VipsImage* out = vips_image_new_from_memory( obuf, W*H*sizeof(MY_PX_TYPE), W, H, 1, VIPS_FORMAT_FLOAT );

  g_timer_start(timer);
  vips_image_write( blurred, out );
  g_timer_stop(timer);
  std::cout<<"padding: "<<padding<<std::endl<<"caching: "<<do_caching<<std::endl<<"blur caching: "<<cache_blur<<std::endl;
  std::cout<<"duration: "<<g_timer_elapsed(timer,NULL)<<std::endl<<std::endl<<"======================"<<std::endl;

  //vips_jpegsave( out, "v1.jpg", "Q", 50, NULL );

  VIPS_UNREF(out);
  VIPS_UNREF(in);
}


void lineblur_tilecache_run()
{
  GTimer* timer = g_timer_new();

  VipsImage* im = vips_image_new_from_memory(
      ibuf, W * H * sizeof( MY_PX_TYPE ), W, H, 1, VIPS_FORMAT_FLOAT );

  VipsImage* out = vips_image_new_from_memory(
      obuf, W * H * sizeof( MY_PX_TYPE ), W, H, 1, VIPS_FORMAT_FLOAT );

  for( int i = 0; i < iterations; i++ ) {
    VipsImage* x;

    //vips_lineblur( im, &x, RADIUS );

    VIPS_UNREF( im );
    im = x;

    if( vips_tilecache( im, &x,
        "access", VIPS_ACCESS_RANDOM,
        "threaded", TRUE,
        "tile-width", 128,
        "tile-height", 128,
        "max-tiles", 4 * im->Xsize / 128,
        NULL ) ) {
      std::cout << "cannot cache" << std::endl;
      return;
    }

    VIPS_UNREF( im );
    im = x;
  }

  g_timer_start( timer );

  if( vips_image_write( im, out ) ) {
    std::cout << "cannot write" << std::endl;
    return;
  }

  g_timer_stop( timer );

  VIPS_UNREF( im );

  std::cout << std::endl << std::endl;
  std::cout << "lineblur plus tilecache" << std::endl;
  std::cout << "duration: "<< g_timer_elapsed( timer, NULL ) <<
      std::endl << std::endl;

  vips_jpegsave( out, "v_tilecache.jpg", "Q", 50, NULL );

  VIPS_UNREF( out );
  free( ibuf );
  free( obuf );
}


int main(int argc, char** argv)
{
  char* fullpath;

  vips_slowop_get_type();
  vips_paddedop_get_type();

  if (vips_init (argv[0]))
    //vips::verror ();
    return 1;

  //im_concurrency_set( 2 );

  ibuf = (MY_PX_TYPE*)malloc(W*H*sizeof(MY_PX_TYPE));
  if( !ibuf ) {
    std::cout<<"cannot allocate memory buffer"<<std::endl;
    return 1;
  }
  obuf = (MY_PX_TYPE*)malloc(W*H*sizeof(MY_PX_TYPE));
  if( !obuf ) {
    std::cout<<"cannot allocate memory buffer"<<std::endl;
    return 1;
  }
  //std::cout<<"allocated two memory buffers of "<<W*H*sizeof(MY_PX_TYPE)/1024/1024<<"MB"<<std::endl;

  //std::cout<<"before fill_pattern()"<<std::endl;
  //fill_pattern(ibuf, W, H);
  //std::cout<<"after fill_pattern()"<<std::endl;

  //im_concurrency_set( 1 );
  //vips_cache_set_trace( true );
  vips_profile_set(true);

  vips_run1(0, false, false);
  vips_run1(0, false, false);
  vips_run1(0, true, false);
  vips_run1(64, false, false);
  vips_run1(64, true, false);
  vips_run1(64, false, true);
  vips_run1(64, true, true);

  vips_shutdown();
  return 0;
}
