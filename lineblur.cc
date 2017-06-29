/* 
 */

/*

    Copyright (C) 2014 Ferrero Andrea

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.


 */




/* Turn on ADDR() range checks.
#define DEBUG 1
 */

//#ifdef HAVE_CONFIG_H
//#include <config.h>
//#endif /*HAVE_CONFIG_H*/
//#include <vips/intl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <iostream>


#include <vips/dispatch.h>

/**/
#define VIPS_TYPE_LINEBLUR (vips_lineblur_get_type())
#define VIPS_LINEBLUR( obj ) \
    (G_TYPE_CHECK_INSTANCE_CAST( (obj), \
        VIPS_TYPE_LINEBLUR, VipsLineBlur ))
#define VIPS_LINEBLUR_CLASS( klass ) \
    (G_TYPE_CHECK_CLASS_CAST( (klass), \
        VIPS_TYPE_LINEBLUR, VipsLineBlurClass))
#define VIPS_IS_LINEBLUR( obj ) \
    (G_TYPE_CHECK_INSTANCE_TYPE( (obj), VIPS_TYPE_LINEBLUR ))
#define VIPS_IS_LINEBLUR_CLASS( klass ) \
    (G_TYPE_CHECK_CLASS_TYPE( (klass), VIPS_TYPE_LINEBLUR ))
#define VIPS_LINEBLUR_GET_CLASS( obj ) \
    (G_TYPE_INSTANCE_GET_CLASS( (obj), \
        VIPS_TYPE_LINEBLUR, VipsLineBlurClass ))

/**/
typedef struct _VipsLineBlur {
  VipsOperation parent_instance;

  /* input image.
   */
  VipsImage* in;
  VipsImage* inv[2];

  VipsImageInfo* info;

  /* The vector of input images.
   */
  VipsImage* out;

  int radius;
} VipsLineBlur;

typedef VipsOperationClass VipsLineBlurClass;

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

G_DEFINE_TYPE( VipsLineBlur, vips_lineblur, VIPS_TYPE_OPERATION );

#ifdef __cplusplus
}
#endif /*__cplusplus*/



/* Run the line blur algorithm
 */
static int
vips_lineblur_gen( VipsRegion *oreg, void *seq, void *a, void *b, gboolean *stop )
{
  VipsRegion **ir = (VipsRegion **) seq;
  VipsLineBlur *lineblur = (VipsLineBlur *) b;

  /* Do the actual processing
   */

  /* Output area we are building.
   */
  const VipsRect *r = &oreg->valid;
  VipsRect s = {
      r->left - lineblur->radius, r->top - lineblur->radius,
      r->width + lineblur->radius*2+1, r->height + lineblur->radius*2+1
  };
  VipsRect rshifted = {
      r->left - lineblur->radius, r->top - lineblur->radius,
      r->width, r->height
  };
  VipsRect r_img = {0, 0, ir[0]->im->Xsize, ir[0]->im->Ysize};
  vips_rect_intersectrect (&s, &r_img, &s);

  if(false && s.top==0 && s.left==0) {
    printf("\n----------------\n");
    printf("radius: %d\n", lineblur->radius);
    printf("r: %d %d %d %d\n", r->top, r->left, r->height, r->width);
    printf("s: %d %d %d %d\n", s.top, s.left, s.height, s.width);
    printf("info: %p\n", lineblur->info);
    if( lineblur->info ) printf("  valid: %d", lineblur->info->valid);
    if( lineblur->info ) printf("  buf: %p", lineblur->info->buf);
    if( lineblur->info )
      printf("  rect: %d %d %d %d",
          lineblur->info->rect.top, lineblur->info->rect.left,
          lineblur->info->rect.height, lineblur->info->rect.width);
    if( lineblur->info )
      printf("  included: %d", (int)vips_rect_includesrect(&(lineblur->info->rect), &rshifted));
    printf("\n");
  }

  int i;
  int x, y, x2;

  if( lineblur->info && lineblur->info->valid==1 && lineblur->info->buf &&
      vips_rect_includesrect(&(lineblur->info->rect), &rshifted) ) {
    // copy buffered pixel data
    size_t pelsz = VIPS_IMAGE_SIZEOF_PEL(oreg->im);
    size_t buflinesz = pelsz * lineblur->info->rect.width;
    size_t outlinesz = pelsz * r->width;
    int dy = rshifted.top - lineblur->info->rect.top;
    int dx = rshifted.left - lineblur->info->rect.left;
    for( y = 0; y < r->height; y++ ) {
      MY_PX_TYPE *p = (MY_PX_TYPE *)(lineblur->info->buf + (y+dy)*buflinesz + dx*pelsz);
      MY_PX_TYPE *q = (MY_PX_TYPE *)VIPS_REGION_ADDR( oreg, r->left, r->top + y );
      memcpy(q, p, outlinesz);
    }
    return 0;
  }

  int R = lineblur->radius*2+1;

  /* Prepare the input images
   */
  if(false && s.top==0 && s.left==0)
    printf("preparing region...\n");
  if( vips_region_prepare( ir[0], &s ) )
    return( -1 );
  if(false && s.top==0 && s.left==0)
    printf("...region prepared\n");

  /* Do the actual processing
   */
  if(false && s.top==0 && s.left==0)
    printf("processing region...\n");
  int line_size = r->width * ir[0]->im->Bands;
  for( y = 0; y < r->height; y++ ) {
    MY_PX_TYPE *p = (MY_PX_TYPE *)
              VIPS_REGION_ADDR( ir[0], s.left, r->top + y );
    MY_PX_TYPE *q = (MY_PX_TYPE *)
              VIPS_REGION_ADDR( oreg, r->left, r->top + y );

    for( x = 0; x < line_size; x++ ) {
      MY_PX_TYPE result = 0;
      for( x2 = x, i = 0; i < R; x2++, i++ ) {
        result += p[x2];
        //if(s.top+y==0 && x+r->left == oreg->im->Xsize-lineblur->radius-1)
        //  std::cout<<"  x2="<<x2<<"  p["<<x2<<"]="<<(float)p[x2]<<std::endl;
      }
      q[x] = result / R;
      //if(s.top+y==0 && x+r->left == oreg->im->Xsize-lineblur->radius-1)
      //   std::cout<<"x="<<x<<"  p["<<x<<"]="<<(float)p[x]<<"  pout["<<x<<"]="<<(float)q[x]<<std::endl;
    }

  }
  if(false && s.top==0 && s.left==0)
    printf("...region processed\n");
  return( 0 );
}


static int
vips_lineblur_build( VipsObject *object )
{
  VipsObjectClass *klass = VIPS_OBJECT_GET_CLASS( object );
  VipsOperation *operation = VIPS_OPERATION( object );
  VipsLineBlur *lineblur = (VipsLineBlur *) object;
  int i;

  //std::cout<<"OK 0"<<std::endl;

  if( VIPS_OBJECT_CLASS( vips_lineblur_parent_class )->build( object ) )
    return( -1 );

  if( vips_image_pio_input( lineblur->in ) ||
      vips_check_coding_known( klass->nickname, lineblur->in ) )
    return( -1 );

  //std::cout<<"OK 1"<<std::endl;

  lineblur->inv[0] = lineblur->in;
  lineblur->inv[1] = NULL;

  /* Get ready to write to @out. @out must be set via g_object_set() so
   * that vips can see the assignment. It'll complain that @out hasn't
   * been set otherwise.
   */
  g_object_set( lineblur, "out", vips_image_new(), NULL );
  //g_object_ref( lineblur->out );

  //std::cout<<"OK 2"<<std::endl;

  /* Set demand hints. 
   */
  if( vips_image_pipeline_array( lineblur->out, VIPS_DEMAND_STYLE_THINSTRIP, lineblur->inv ) )
    return( -1 );

  //std::cout<<"OK 3"<<std::endl;

  vips_image_init_fields( lineblur->out,
      lineblur->in->Xsize, lineblur->in->Ysize,
      lineblur->in->Bands, lineblur->in->BandFmt,
      lineblur->in->Coding,
      lineblur->in->Type,
      1.0, 1.0);

  //std::cout<<"OK 4"<<std::endl;
  //std::cout<<"lineblur->out="<<lineblur->out<<std::endl;

 int nimg = 1;
  if(nimg > 0) {
    if( vips_image_generate( lineblur->out,
        vips_start_many, vips_lineblur_gen, vips_stop_many,
        lineblur->inv, lineblur ) )
      return( -1 );
  } else {
    if( vips_image_generate( lineblur->out,
        NULL, vips_lineblur_gen, NULL, NULL, lineblur ) )
      return( -1 );
  }

  //std::cout<<"OK 5"<<std::endl;

  return( 0 );
}


static void
vips_lineblur_class_init( VipsLineBlurClass *klass )
{
  GObjectClass *gobject_class = G_OBJECT_CLASS( klass );
  VipsObjectClass *vobject_class = VIPS_OBJECT_CLASS( klass );
  VipsOperationClass *operation_class = VIPS_OPERATION_CLASS( klass );

  gobject_class->set_property = vips_object_set_property;
  gobject_class->get_property = vips_object_get_property;

  vobject_class->nickname = "mylineblur";
  vobject_class->description = _( "mylineblur" );
  vobject_class->build = vips_lineblur_build;

  operation_class->flags = VIPS_OPERATION_SEQUENTIAL_UNBUFFERED/*+VIPS_OPERATION_NOCACHE*/;

  int argid = 0;

  VIPS_ARG_IMAGE( klass, "in", argid,
      _( "Input" ),
      _( "Input image" ),
      VIPS_ARGUMENT_REQUIRED_INPUT,
      G_STRUCT_OFFSET( VipsLineBlur, in ) );
  argid += 1;

  VIPS_ARG_IMAGE( klass, "out", argid, 
      _( "Output" ),
      _( "Output image" ),
      VIPS_ARGUMENT_REQUIRED_OUTPUT,
      G_STRUCT_OFFSET( VipsLineBlur, out ) );
  argid += 1;

  VIPS_ARG_INT( klass, "radius", argid,
      _( "Radius" ),
      _( "Blur radius" ),
      VIPS_ARGUMENT_REQUIRED_INPUT,
      G_STRUCT_OFFSET( VipsLineBlur, radius ),
      0, 10000000, 0);
  argid += 1;

  VIPS_ARG_POINTER( klass, "info", argid,
        _( "Info" ),
        _( "Image info" ),
        VIPS_ARGUMENT_REQUIRED_INPUT,
        G_STRUCT_OFFSET( VipsLineBlur, info ) );
  argid += 1;
}

static void
vips_lineblur_init( VipsLineBlur *lineblur )
{
  lineblur->in = NULL;
  lineblur->info = NULL;
}

/**
 * vips_lineblur:
 * @in: input image
 * @out: output image
 * @...: %NULL-terminated list of optional named arguments
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_lineblur_int( VipsImage* in, VipsImage **out, int radius, VipsImageInfo* info)
{
  int result;

  if( info ) info->xpadding = info->ypadding = radius;

  //std::cout<<"vips_lineblur: in="<<in<<std::endl;
  result = vips_call( "mylineblur", in, out, radius, info, NULL );
  //va_start( ap, radius );
  //result = vips_call_split( "mylineblur", ap, in, out, radius );
  //va_end( ap );

  return( result );
}


int
vips_lineblur( VipsImage* in, VipsImage **out, int radius, VipsImageInfo* info=NULL)
{
  VipsImage* extended = in;
  //g_object_ref(extended);

  //std::cout<<"lineblur in size: "<<in->Xsize<<","<<in->Ysize<<std::endl;

  if( radius > 0) {
    if( vips_embed( in, &extended, radius, radius,
        in->Xsize + radius*2 + 1, in->Ysize + radius*2 + 1,
        "extend", VIPS_EXTEND_COPY, NULL ) )
      return( -1 );
  } else {
    g_object_ref(extended);
  }
  //std::cout<<"lineblur extended size: "<<extended->Xsize<<","<<extended->Ysize<<std::endl;


  VipsImage* blurred = extended;
  if( vips_lineblur_int(extended, &blurred, radius, info) ) return( -1 );
  g_object_unref(extended);
  //std::cout<<"lineblur blurred size: "<<blurred->Xsize<<","<<blurred->Ysize<<std::endl;

  if( radius > 0) {
    if( vips_crop( blurred, out, radius, radius,
        in->Xsize, in->Ysize, NULL ) )
      return( -1 );
    g_object_unref(blurred);
  } else {
    *out = blurred;
  }

  //std::cout<<"lineblur out size: "<<(*out)->Xsize<<","<<(*out)->Ysize<<std::endl;


  return( 0 );
}
