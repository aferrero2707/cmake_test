#include<iostream>
#include <stdio.h>
#include <vips/vips.h>
#include <stdlib.h>

typedef float MY_PX_TYPE;

#define _(a) (a)

#include "imgtree.cc"

#include "lineblur.cc"

#define RADIUS 50
const size_t W=10000-100;
const size_t H=W;
const int iterations = 2;



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


void lineblur_simple(MY_PX_TYPE* ibuf, MY_PX_TYPE* obuf, size_t W, size_t H, int radius)
{
  //std::cout<<"lineblur_simple("<<W<<","<<H<<","<<radius<<")"<<std::endl;
  float R = radius*2 + 1;
  float R2 = radius*2;
  int cstart;
  int cend;
  int cend2;
  int i, row, col, col2;
  for(row = 0; row < H; row++) {
    //if(row%10 == 0)std::cout<<"row="<<row<<std::endl;
    MY_PX_TYPE* pi = ibuf + row*W;
    MY_PX_TYPE* po = obuf + row*W;
    for(col = 0; col < W; col++) {
      MY_PX_TYPE result = 0;
      cstart = col-radius;
      cend = cend2 = col+radius; if(cend >= W) cend = W-1;
      for(col2 = col-radius; col2 < 0; col2++) {
        result += pi[0];
      }
      for(; col2 <= cend; col2++) {
        result += pi[col2];
      }
      for(; col2 <= cend2; col2++) {
        result += pi[W-1];
      }
      po[col] = result / R;
    }
  }
}


void lineblur_simple_run()
{
  GTimer* timer = g_timer_new();

  MY_PX_TYPE* ibuf = (MY_PX_TYPE*)malloc(W*H*sizeof(MY_PX_TYPE));
  if( !ibuf ) {
    std::cout<<"cannot allocate memory buffer"<<std::endl;
    return;
  }
  MY_PX_TYPE* obuf = (MY_PX_TYPE*)malloc(W*H*sizeof(MY_PX_TYPE));
  if( !obuf ) {
    std::cout<<"cannot allocate memory buffer"<<std::endl;
    return;
  }
  //std::cout<<"allocated two memory buffers of "<<W*H*sizeof(MY_PX_TYPE)/1024/1024<<"MB"<<std::endl;

  //std::cout<<"before fill_pattern()"<<std::endl;
  fill_pattern(ibuf, W, H);
  //std::cout<<"after fill_pattern()"<<std::endl;

  VipsImage* in = vips_image_new_from_memory( ibuf, W*H*sizeof(MY_PX_TYPE), W, H, 1, VIPS_FORMAT_FLOAT );
  vips_jpegsave( in, "a.jpg", "Q", 50, NULL );
  VIPS_UNREF(in);

  std::cout<<"before lineblur_simple(1)"<<std::endl;
  g_timer_start(timer);
  for(int i = 0; i < iterations; i++) {
    lineblur_simple(ibuf, obuf, W, H, RADIUS);
    MY_PX_TYPE* tbuf = obuf;
    obuf = ibuf;
    ibuf = tbuf;
  }
  g_timer_stop(timer);
  std::cout<<"after lineblur_simple(1)"<<std::endl;
  std::cout<<"duration: "<<g_timer_elapsed(timer,NULL)<<std::endl;

  VipsImage* out = vips_image_new_from_memory( ibuf, W*H*sizeof(MY_PX_TYPE), W, H, 1, VIPS_FORMAT_FLOAT );
  vips_jpegsave( out, "b.jpg", "Q", 50, NULL );
  VIPS_UNREF(out);
}




void lineblur_vips1_run()
{
  GTimer* timer = g_timer_new();

  MY_PX_TYPE* ibuf = (MY_PX_TYPE*)malloc(W*H*sizeof(MY_PX_TYPE));
  if( !ibuf ) {
    std::cout<<"cannot allocate memory buffer"<<std::endl;
    return;
  }
  MY_PX_TYPE* obuf = (MY_PX_TYPE*)malloc(W*H*sizeof(MY_PX_TYPE));
  if( !obuf ) {
    std::cout<<"cannot allocate memory buffer"<<std::endl;
    return;
  }
  //std::cout<<"allocated two memory buffers of "<<W*H*sizeof(MY_PX_TYPE)/1024/1024<<"MB"<<std::endl;

  //std::cout<<"before fill_pattern()"<<std::endl;
  fill_pattern(ibuf, W, H);
  //std::cout<<"after fill_pattern()"<<std::endl;

  VipsImage* in = vips_image_new_from_memory( ibuf, W*H*sizeof(MY_PX_TYPE), W, H, 1, VIPS_FORMAT_FLOAT );
  VipsImage* in2 = in;
  g_object_ref(in);
  VipsImage* ti = NULL;
  for(int i = 0; i < iterations; i++) {
    vips_lineblur(in2, &ti, RADIUS);
    VIPS_UNREF(in2);
    in2 = ti;
  }

  VipsImage* out = vips_image_new_from_memory( obuf, W*H*sizeof(MY_PX_TYPE), W, H, 1, VIPS_FORMAT_FLOAT );

  g_timer_start(timer);
  vips_image_write( in2, out );
  g_timer_stop(timer);
  std::cout<<"duration: "<<g_timer_elapsed(timer,NULL)<<std::endl<<std::endl<<"======================"<<std::endl;

  vips_jpegsave( out, "c.jpg", "Q", 50, NULL );

  VIPS_UNREF(out);
  VIPS_UNREF(in);
}




void lineblur_vips2_run()
{
  GTimer* timer = g_timer_new();

  MY_PX_TYPE* ibuf = (MY_PX_TYPE*)malloc(W*H*sizeof(MY_PX_TYPE));
  if( !ibuf ) {
    std::cout<<"cannot allocate memory buffer"<<std::endl;
    return;
  }
  MY_PX_TYPE* obuf = (MY_PX_TYPE*)malloc(W*H*sizeof(MY_PX_TYPE));
  if( !obuf ) {
    std::cout<<"cannot allocate memory buffer"<<std::endl;
    return;
  }
  //std::cout<<"allocated two memory buffers of "<<W*H*sizeof(MY_PX_TYPE)/1024/1024<<"MB"<<std::endl;

  //std::cout<<"before fill_pattern()"<<std::endl;
  fill_pattern(ibuf, W, H);
  //std::cout<<"after fill_pattern()"<<std::endl;

  VipsImage* in = vips_image_new_from_memory( ibuf, W*H*sizeof(MY_PX_TYPE), W, H, 1, VIPS_FORMAT_FLOAT );
  VipsImage* out = vips_image_new_from_memory( obuf, W*H*sizeof(MY_PX_TYPE), W, H, 1, VIPS_FORMAT_FLOAT );

  VipsImage* in2 = in;
  g_object_ref(in);
  VipsImage* ti = NULL;
  g_timer_start(timer);
  for(int i = 0; i < iterations; i++) {
    vips_lineblur(in, &ti, RADIUS);
    vips_image_write( ti, out );
    VIPS_UNREF(ti);
    ti = in;
    in = out;
    out = ti;
  }
  g_timer_stop(timer);
  std::cout<<"duration: "<<g_timer_elapsed(timer,NULL)<<std::endl<<std::endl<<std::endl;

  vips_jpegsave( in, "c.jpg", "Q", 50, NULL );

  VIPS_UNREF(out);
  VIPS_UNREF(in);
}




void lineblur_vips3_run()
{
  GTimer* timer = g_timer_new();

  MY_PX_TYPE* ibuf = (MY_PX_TYPE*)malloc(W*H*sizeof(MY_PX_TYPE));
  if( !ibuf ) {
    std::cout<<"cannot allocate memory buffer"<<std::endl;
    return;
  }
  //std::cout<<"allocated two memory buffers of "<<W*H*sizeof(MY_PX_TYPE)/1024/1024<<"MB"<<std::endl;

  //std::cout<<"before fill_pattern()"<<std::endl;
  fill_pattern(ibuf, W, H);
  //std::cout<<"after fill_pattern()"<<std::endl;

  VipsImage* in = vips_image_new_from_memory( ibuf, W*H*sizeof(MY_PX_TYPE), W, H, 1, VIPS_FORMAT_FLOAT );

  VipsImageTree* tree = new VipsImageTree;
  tree->root = NULL;

  TreeImage* nodes[1000];
  int nnodes = 0;

  VipsImage* in2 = in;
  g_object_ref(in);
  VipsImage* ti = NULL;
  for(int i = 0; i < iterations; i++) {
    VipsImageInfo* info = new VipsImageInfo; info->valid = 0;
    vips_lineblur(in2, &ti, RADIUS, info);

    TreeImage* treeimg = new TreeImage;
    treeimg->image = ti;
    treeimg->info = info;
    treeimg->nparents = 0;

    nodes[nnodes] = treeimg;
    nnodes += 1;
    in2 = ti;
  }

  // Fill the tree
  VipsImageTreeNode* last_node = NULL;
  for(int i = nnodes-1; i >= 0; i--) {
    if( !last_node ) {
      tree->root = new_image_tree_node(nodes[i]);
      last_node = tree->root;
    } else {
      VipsImageTreeNode* new_node = image_tree_insert_node(last_node,nodes[i]);
      last_node = new_node;
    }
  }

  nbufactive = 0;
  VipsRect area = {0, 0, in->Xsize, in->Ysize};
  g_timer_start(timer);
  void* obuf = image_tree_sink_memory( tree, &area );
  g_timer_stop(timer);
  std::cout<<"duration: "<<g_timer_elapsed(timer,NULL)<<std::endl<<std::endl<<std::endl;

  VipsImage* out = vips_image_new_from_memory( obuf, W*H*sizeof(MY_PX_TYPE), W, H, 1, VIPS_FORMAT_FLOAT );
  //vips_image_write(tree->root->image->image, out);
  vips_jpegsave( out, "d.jpg", "Q", 50, NULL );

  VIPS_UNREF(out);
  VIPS_UNREF(in);
}


int main(int argc, char** argv)
{
  char* fullpath;

  vips_lineblur_get_type();

  if (vips_init (argv[0]))
    //vips::verror ();
    return 1;

  im_concurrency_set( 1 );
  //vips_cache_set_trace( true );

  //lineblur_vips1_run();
  lineblur_vips2_run();
  lineblur_vips3_run();
  lineblur_simple_run();

  /*
  std::cout<<"before lineblur_simple(2)"<<std::endl;
  g_timer_start(timer);
  lineblur_simple(ibuf, obuf, W, H, RADIUS);
  g_timer_stop(timer);
  std::cout<<"after lineblur_simple(2)"<<std::endl;
  std::cout<<"duration: "<<g_timer_elapsed(timer,NULL)<<std::endl;
  g_timer_reset(timer);
  */

  vips_shutdown();
  return 0;
}
