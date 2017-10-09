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
#define VIPS_TYPE_PADDEDOP (vips_paddedop_get_type())
#define VIPS_PADDEDOP( obj ) \
    (G_TYPE_CHECK_INSTANCE_CAST( (obj), \
        VIPS_TYPE_PADDEDOP, VipsPaddedOp ))
#define VIPS_PADDEDOP_CLASS( klass ) \
    (G_TYPE_CHECK_CLASS_CAST( (klass), \
        VIPS_TYPE_PADDEDOP, VipsPaddedOpClass))
#define VIPS_IS_PADDEDOP( obj ) \
    (G_TYPE_CHECK_INSTANCE_TYPE( (obj), VIPS_TYPE_PADDEDOP ))
#define VIPS_IS_PADDEDOP_CLASS( klass ) \
    (G_TYPE_CHECK_CLASS_TYPE( (klass), VIPS_TYPE_PADDEDOP ))
#define VIPS_PADDEDOP_GET_CLASS( obj ) \
    (G_TYPE_INSTANCE_GET_CLASS( (obj), \
        VIPS_TYPE_PADDEDOP, VipsPaddedOpClass ))

/**/
typedef struct _VipsPaddedOp {
  VipsOperation parent_instance;

  /* input image.
   */
  VipsImage* in;
  VipsImage* inv[2];

  VipsImageInfo* info;

  /* The vector of input images.
   */
  VipsImage* out;

  int padding;
} VipsPaddedOp;

typedef VipsOperationClass VipsPaddedOpClass;

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

G_DEFINE_TYPE( VipsPaddedOp, vips_paddedop, VIPS_TYPE_OPERATION );

#ifdef __cplusplus
}
#endif /*__cplusplus*/



/* Run the line blur algorithm
 */
static int
vips_paddedop_gen( VipsRegion *oreg, void *seq, void *a, void *b, gboolean *stop )
{
  VipsRegion **ir = (VipsRegion **) seq;
  VipsPaddedOp *paddedop = (VipsPaddedOp *) b;
  g_assert(paddedop);
  VipsImageInfo* info = paddedop->info;


  /* Output area we are building.
   */
  const VipsRect *r = &oreg->valid;
  VipsRect s = {
      r->left - paddedop->padding, r->top - paddedop->padding,
      r->width + paddedop->padding*2, r->height + paddedop->padding*2
  };
  VipsRect r_img = {0, 0, ir[0]->im->Xsize, ir[0]->im->Ysize};
  vips_rect_intersectrect (&s, &r_img, &s);

  if( false && r->top == 0 && r->left == 0 )
    std::cout<<"[paddedop] output region: top="<<r->top<<" left="<<r->left
    <<" width="<<r->width<<" height="<<r->height<<std::endl;

  /* Prepare the input images
   */
  if( vips_region_prepare( ir[0], &s ) )
    return( -1 );

  int i;
  int x, y, x2, y2;

  /* Do the actual processing
   */
  int line_size = r->width * ir[0]->im->Bands;
  for( y = 0; y < r->height; y++ ) {
    MY_PX_TYPE *p = (MY_PX_TYPE *)
              VIPS_REGION_ADDR( ir[0], r->left, r->top + y );
    MY_PX_TYPE *q = (MY_PX_TYPE *)
              VIPS_REGION_ADDR( oreg, r->left, r->top + y );
    for( x = 0; x < line_size; x++ ) {
      q[x] = p[x];
    }
  }

  return( 0 );
}


static int
vips_paddedop_build( VipsObject *object )
{
  VipsObjectClass *klass = VIPS_OBJECT_GET_CLASS( object );
  VipsOperation *operation = VIPS_OPERATION( object );
  VipsPaddedOp *paddedop = (VipsPaddedOp *) object;
  int i;

  //std::cout<<"OK 0"<<std::endl;

  if( VIPS_OBJECT_CLASS( vips_paddedop_parent_class )->build( object ) )
    return( -1 );

  if( vips_image_pio_input( paddedop->in ) ||
      vips_check_coding_known( klass->nickname, paddedop->in ) )
    return( -1 );

  //std::cout<<"OK 1"<<std::endl;

  paddedop->inv[0] = paddedop->in;
  paddedop->inv[1] = NULL;

  /* Get ready to write to @out. @out must be set via g_object_set() so
   * that vips can see the assignment. It'll complain that @out hasn't
   * been set otherwise.
   */
  g_object_set( paddedop, "out", vips_image_new(), NULL );
  //g_object_ref( paddedop->out );

  //std::cout<<"OK 2"<<std::endl;

  /* Set demand hints. 
   */
  if( vips_image_pipeline_array( paddedop->out, VIPS_DEMAND_STYLE_THINSTRIP, paddedop->inv ) )
    return( -1 );

  //std::cout<<"OK 3"<<std::endl;

  vips_image_init_fields( paddedop->out,
      paddedop->in->Xsize, paddedop->in->Ysize,
      paddedop->in->Bands, paddedop->in->BandFmt,
      paddedop->in->Coding,
      paddedop->in->Type,
      1.0, 1.0);

  //std::cout<<"OK 4"<<std::endl;
  //std::cout<<"paddedop->out="<<paddedop->out<<std::endl;

  int nimg = 1;
  if(nimg > 0) {
    if( vips_image_generate( paddedop->out,
        vips_start_many, vips_paddedop_gen, vips_stop_many,
        paddedop->inv, paddedop ) )
      return( -1 );
  } else {
    if( vips_image_generate( paddedop->out,
        NULL, vips_paddedop_gen, NULL, NULL, paddedop ) )
      return( -1 );
  }

  //std::cout<<"OK 5"<<std::endl;

  return( 0 );
}


static void
vips_paddedop_class_init( VipsPaddedOpClass *klass )
{
  GObjectClass *gobject_class = G_OBJECT_CLASS( klass );
  VipsObjectClass *vobject_class = VIPS_OBJECT_CLASS( klass );
  VipsOperationClass *operation_class = VIPS_OPERATION_CLASS( klass );

  gobject_class->set_property = vips_object_set_property;
  gobject_class->get_property = vips_object_get_property;

  vobject_class->nickname = "mypaddedop";
  vobject_class->description = _( "mypaddedop" );
  vobject_class->build = vips_paddedop_build;

  operation_class->flags = VIPS_OPERATION_SEQUENTIAL_UNBUFFERED/*+VIPS_OPERATION_NOCACHE*/;

  int argid = 0;

  VIPS_ARG_IMAGE( klass, "in", argid,
      _( "Input" ),
      _( "Input image" ),
      VIPS_ARGUMENT_REQUIRED_INPUT,
      G_STRUCT_OFFSET( VipsPaddedOp, in ) );
  argid += 1;

  VIPS_ARG_IMAGE( klass, "out", argid, 
      _( "Output" ),
      _( "Output image" ),
      VIPS_ARGUMENT_REQUIRED_OUTPUT,
      G_STRUCT_OFFSET( VipsPaddedOp, out ) );
  argid += 1;

  VIPS_ARG_INT( klass, "padding", argid,
      _( "Padding" ),
      _( "Region padding" ),
      VIPS_ARGUMENT_REQUIRED_INPUT,
      G_STRUCT_OFFSET( VipsPaddedOp, padding ),
      0, 10000000, 0);
  argid += 1;

  VIPS_ARG_POINTER( klass, "info", argid,
        _( "Info" ),
        _( "Image info" ),
        VIPS_ARGUMENT_REQUIRED_INPUT,
        G_STRUCT_OFFSET( VipsPaddedOp, info ) );
  argid += 1;
}

static void
vips_paddedop_init( VipsPaddedOp *paddedop )
{
  paddedop->in = NULL;
  paddedop->padding = 0;
  paddedop->info = NULL;
}

/**
 * vips_paddedop:
 * @in: input image
 * @out: output image
 * @...: %NULL-terminated list of optional named arguments
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_paddedop( VipsImage* in, VipsImage **out, int padding, VipsImageInfo* info)
{
  int result;

  if( info ) {
    info->xpadding = 0;
    info->ypadding = 0;
  }

  //std::cout<<"vips_paddedop: in="<<in<<std::endl;
  result = vips_call( "mypaddedop", in, out, padding, info, NULL );
  //va_start( ap, radius );
  //result = vips_call_split( "mypaddedop", ap, in, out, radius );
  //va_end( ap );

  return( result );
}
