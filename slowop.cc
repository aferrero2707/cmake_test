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
#define VIPS_TYPE_SLOWOP (vips_slowop_get_type())
#define VIPS_SLOWOP( obj ) \
    (G_TYPE_CHECK_INSTANCE_CAST( (obj), \
        VIPS_TYPE_SLOWOP, VipsSlowOp ))
#define VIPS_SLOWOP_CLASS( klass ) \
    (G_TYPE_CHECK_CLASS_CAST( (klass), \
        VIPS_TYPE_SLOWOP, VipsSlowOpClass))
#define VIPS_IS_SLOWOP( obj ) \
    (G_TYPE_CHECK_INSTANCE_TYPE( (obj), VIPS_TYPE_SLOWOP ))
#define VIPS_IS_SLOWOP_CLASS( klass ) \
    (G_TYPE_CHECK_CLASS_TYPE( (klass), VIPS_TYPE_SLOWOP ))
#define VIPS_SLOWOP_GET_CLASS( obj ) \
    (G_TYPE_INSTANCE_GET_CLASS( (obj), \
        VIPS_TYPE_SLOWOP, VipsSlowOpClass ))

/**/
typedef struct _VipsSlowOp {
  VipsOperation parent_instance;

  /* input image.
   */
  VipsImage* in;
  VipsImage* inv[2];

  VipsImageInfo* info;

  /* The vector of input images.
   */
  VipsImage* out;

  double gamma;
} VipsSlowOp;

typedef VipsOperationClass VipsSlowOpClass;

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

G_DEFINE_TYPE( VipsSlowOp, vips_slowop, VIPS_TYPE_OPERATION );

#ifdef __cplusplus
}
#endif /*__cplusplus*/



/* Run the line blur algorithm
 */
static int
vips_slowop_gen( VipsRegion *oreg, void *seq, void *a, void *b, gboolean *stop )
{
  VipsRegion **ir = (VipsRegion **) seq;
  VipsSlowOp *slowop = (VipsSlowOp *) b;
  g_assert(slowop);
  VipsImageInfo* info = slowop->info;


  /* Output area we are building.
   */
  VipsRect *r = &oreg->valid;

  if( false && r->top == 0 && r->left == 0 )
    std::cout<<"[slowop] output region: top="<<r->top<<" left="<<r->left
    <<" width="<<r->width<<" height="<<r->height<<std::endl;

  /* Prepare the input images
   */
  if( vips_region_prepare( ir[0], r ) )
    return( -1 );

  int i;
  int x, y, x2, y2, k;
  float RGB[3];

  /* Do the actual processing
   */
  int line_size = r->width * ir[0]->im->Bands;
  for( y = 0; y < r->height; y++ ) {
    MY_PX_TYPE *p = (MY_PX_TYPE *)
              VIPS_REGION_ADDR( ir[0], r->left, r->top + y );
    MY_PX_TYPE *q = (MY_PX_TYPE *)
              VIPS_REGION_ADDR( oreg, r->left, r->top + y );
    for( x = 0; x < line_size; x+=3 ) {
      RGB[0] = p[x];
      RGB[1] = p[x+1];
      RGB[2] = p[x+2];
      for(i=0; i<20;i++) {
      for( k=0; k < 3; k++) {
        RGB[k] = powf( RGB[k], slowop->gamma );
      }
      }
      q[x] = RGB[0];
      q[x+1] = RGB[1];
      q[x+2] = RGB[2];
      //q[x] = p[x]; //if( (x%100) == 0 )
      //for(i=0; i<50;i++) q[x] = powf( p[x], slowop->gamma );
      //g_usleep(10);
    }
  }

  return( 0 );
}


static int
vips_slowop_build( VipsObject *object )
{
  VipsObjectClass *klass = VIPS_OBJECT_GET_CLASS( object );
  VipsOperation *operation = VIPS_OPERATION( object );
  VipsSlowOp *slowop = (VipsSlowOp *) object;
  int i;

  //std::cout<<"OK 0"<<std::endl;

  if( VIPS_OBJECT_CLASS( vips_slowop_parent_class )->build( object ) )
    return( -1 );

  if( vips_image_pio_input( slowop->in ) ||
      vips_check_coding_known( klass->nickname, slowop->in ) )
    return( -1 );

  //std::cout<<"OK 1"<<std::endl;

  slowop->inv[0] = slowop->in;
  slowop->inv[1] = NULL;

  /* Get ready to write to @out. @out must be set via g_object_set() so
   * that vips can see the assignment. It'll complain that @out hasn't
   * been set otherwise.
   */
  g_object_set( slowop, "out", vips_image_new(), NULL );
  //g_object_ref( slowop->out );

  //std::cout<<"OK 2"<<std::endl;

  /* Set demand hints. 
   */
  if( vips_image_pipeline_array( slowop->out, VIPS_DEMAND_STYLE_THINSTRIP, slowop->inv ) )
    return( -1 );

  //std::cout<<"OK 3"<<std::endl;

  vips_image_init_fields( slowop->out,
      slowop->in->Xsize, slowop->in->Ysize,
      slowop->in->Bands, slowop->in->BandFmt,
      slowop->in->Coding,
      slowop->in->Type,
      1.0, 1.0);

  //std::cout<<"OK 4"<<std::endl;
  //std::cout<<"slowop->out="<<slowop->out<<std::endl;

  int nimg = 1;
  if(nimg > 0) {
    if( vips_image_generate( slowop->out,
        vips_start_many, vips_slowop_gen, vips_stop_many,
        slowop->inv, slowop ) )
      return( -1 );
  } else {
    if( vips_image_generate( slowop->out,
        NULL, vips_slowop_gen, NULL, NULL, slowop ) )
      return( -1 );
  }

  //std::cout<<"OK 5"<<std::endl;

  return( 0 );
}


static void
vips_slowop_class_init( VipsSlowOpClass *klass )
{
  GObjectClass *gobject_class = G_OBJECT_CLASS( klass );
  VipsObjectClass *vobject_class = VIPS_OBJECT_CLASS( klass );
  VipsOperationClass *operation_class = VIPS_OPERATION_CLASS( klass );

  gobject_class->set_property = vips_object_set_property;
  gobject_class->get_property = vips_object_get_property;

  vobject_class->nickname = "myslowop";
  vobject_class->description = _( "myslowop" );
  vobject_class->build = vips_slowop_build;

  operation_class->flags = VIPS_OPERATION_SEQUENTIAL_UNBUFFERED/*+VIPS_OPERATION_NOCACHE*/;

  int argid = 0;

  VIPS_ARG_IMAGE( klass, "in", argid,
      _( "Input" ),
      _( "Input image" ),
      VIPS_ARGUMENT_REQUIRED_INPUT,
      G_STRUCT_OFFSET( VipsSlowOp, in ) );
  argid += 1;

  VIPS_ARG_IMAGE( klass, "out", argid, 
      _( "Output" ),
      _( "Output image" ),
      VIPS_ARGUMENT_REQUIRED_OUTPUT,
      G_STRUCT_OFFSET( VipsSlowOp, out ) );
  argid += 1;

  VIPS_ARG_POINTER( klass, "info", argid,
        _( "Info" ),
        _( "Image info" ),
        VIPS_ARGUMENT_REQUIRED_INPUT,
        G_STRUCT_OFFSET( VipsSlowOp, info ) );
  argid += 1;

  VIPS_ARG_DOUBLE( klass, "gamma", argid,
      _( "Gamma" ),
      _( "Exponent value" ),
      VIPS_ARGUMENT_REQUIRED_INPUT,
      G_STRUCT_OFFSET( VipsSlowOp, gamma ),
      0, 10000000, 1);
  argid += 1;
}

static void
vips_slowop_init( VipsSlowOp *slowop )
{
  slowop->in = NULL;
  slowop->info = NULL;
  slowop->gamma = 1;
}

/**
 * vips_slowop:
 * @in: input image
 * @out: output image
 * @...: %NULL-terminated list of optional named arguments
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_slowop( VipsImage* in, VipsImage **out, double gamma, VipsImageInfo* info)
{
  int result;

  if( info ) {
    info->xpadding = 0;
    info->ypadding = 0;
  }

  //std::cout<<"vips_slowop: in="<<in<<std::endl;
  result = vips_call( "myslowop", in, out, info, gamma, NULL );
  //va_start( ap, radius );
  //result = vips_call_split( "myslowop", ap, in, out, radius );
  //va_end( ap );

  return( result );
}
