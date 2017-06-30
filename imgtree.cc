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


int nbufactive;



typedef struct _VipsImageInfo {
  char* buf;
  VipsRect rect;
  int xpadding, ypadding;
  int valid;
  int refcount;
} VipsImageInfo;


typedef struct _TreeImage {
  VipsImage* image;
  VipsImageInfo* info;
  int nparents;
} TreeImage;


typedef struct _VipsImageTreeNode {
  TreeImage* image;
  int nparents;
  _VipsImageTreeNode* parent;
  int nchildren;
  _VipsImageTreeNode** children;
  int nchildren_indirect;
  _VipsImageTreeNode** children_indirect;
} VipsImageTreeNode;


typedef struct _VipsImageTree {
  VipsImageTreeNode* root;
} VipsImageTree;


VipsImageTreeNode* new_image_tree_node(TreeImage* img)
{
  VipsImageTreeNode* elt = (VipsImageTreeNode*)malloc(sizeof(VipsImageTreeNode));
  if( !elt ) return NULL;
  elt->image = img;
  elt->nparents = 0;
  elt->nchildren = 0;
  elt->nchildren_indirect = 0;
  elt->parent = NULL;
  elt->children = NULL;
  elt->children_indirect = NULL;
  return( elt );
}


VipsImageTreeNode* image_tree_insert_node(VipsImageTreeNode* parent, TreeImage* img)
{
  VipsImageTreeNode* node = new_image_tree_node(img);
  node->parent = parent;
  img->nparents += 1;
  parent->nchildren += 1;
  parent->children = (_VipsImageTreeNode**)realloc(parent->children, sizeof(_VipsImageTreeNode*)*parent->nchildren);
  parent->children[parent->nchildren-1] = node;
  return( node );
}


VipsImageTreeNode* image_tree_insert_indirect_node(VipsImageTreeNode* parent, VipsImageTreeNode* node)
{
  if( !parent ) return NULL;
  parent->nchildren_indirect += 1;
  parent->children_indirect =
      (_VipsImageTreeNode**)realloc(parent->children_indirect,
          sizeof(_VipsImageTreeNode*)*parent->nchildren_indirect);
  parent->children_indirect[parent->nchildren_indirect-1] = node;
  return( node );
}



int image_tree_sink_node(VipsImageTreeNode* node, VipsRect* area, int alloc=1, char* outbuf=NULL)
{
  VipsImageInfo* info = node->image->info;
  //printf("image_tree_sink_node(%p,%d,%p)\n", node, alloc, outbuf);
  //printf("  area: %d,%d+%d+%d\n",area->width,area->height,area->left,area->top);
  //printf("  info: %p\n",info);
  if( info->valid && info->buf && vips_rect_includesrect(&(info->rect), area) ) {
    // this node has already been processed, return immediately
    return( 0 );
  }

  // Prepare children nodes
  for(int i = 0; i < node->nchildren; i++) {
    VipsRect new_area = {
        area->left - info->xpadding, area->top - info->ypadding,
        area->width + info->xpadding*2 + 1, area->height + info->ypadding*2 + 1
    };
    //printf("preparing node %d -> %p\n", i, node->children[i]);
    if( image_tree_sink_node(node->children[i], &new_area, alloc) )
      return( -1 );
  }

  info->rect.left = 0;
  info->rect.top = 0;
  info->rect.width = node->image->image->Xsize;
  info->rect.height = node->image->image->Ysize;
  vips_rect_intersectrect(&(info->rect), area, &(info->rect));

  size_t bufsz = VIPS_IMAGE_SIZEOF_PEL(node->image->image) * info->rect.width * info->rect.height;

  if( !outbuf ) {
    // no pre-allocated buffer is given, the image is an intermediate node and the
    // buffer has to be allocated internally
    // allocate memory buffer
    info->buf = (alloc==0) ? NULL : (char*)malloc( bufsz );

    if( !info->buf ) {
      // not enough memory to allocate buffer or allocation not requested
      // this is not a fatal error, instead the pixels will be computed on-the-fly
      info->valid = 0;
      // we need to transfer the ownership of the children nodes to the parent
      for(int i = 0; i < node->nchildren; i++) {
        //printf("preparing node %d -> %p\n", i, node->children[i]);
        image_tree_insert_indirect_node( node->parent, node->children[i]);
      }
      return 0;
    }
    outbuf = info->buf;
    nbufactive += 1;
    //printf("  info=%p buffer allocated, nbufactive=%d\n", info, nbufactive);
  } else {
    // we are computing the root node, make sure that the internal buffer is NULL
    info->buf = NULL;
    info->valid = 0;
  }


  VipsImage* cropped = NULL;
  bool do_crop = false;
  if( do_crop ) {
  if( vips_crop( node->image->image, &cropped,
      info->rect.left, info->rect.top,
      info->rect.width, info->rect.height, NULL ) )
    return( -1 );
  } else {
    cropped = node->image->image;
    g_object_ref(cropped);
  }


  // write image data into buffer
  VipsImage* membuf = vips_image_new_from_memory( outbuf, bufsz,
      info->rect.width, info->rect.height, node->image->image->Bands, node->image->image->BandFmt );
  if( !membuf ) {
    if( info->buf ) {
      free( info->buf );
      info->buf = NULL;
      info->valid = 0;
    }
    VIPS_UNREF(cropped);
    return( -1 );
  }
  //printf("  writing image...\n");
  if( vips_image_write( cropped, membuf ) ) {
    if( info->buf ) {
      free( info->buf );
      info->buf = NULL;
      info->valid = 0;
    }
    VIPS_UNREF(cropped);
    VIPS_UNREF(membuf);
    return( -1 );
  }
  //printf("  ...writing done.\n");
  VIPS_UNREF(cropped);
  VIPS_UNREF(membuf);

  if( info->buf ) {
    info->valid = 1;
    info->refcount = node->image->nparents;
    //printf("  info=%p  refcount set to %d\n", info, info->refcount);
  }

  // Decrease reference count of children buffers, and free the orphaned ones
  for(int i = 0; i < node->nchildren; i++) {
    VipsImageTreeNode* child = node->children[i];
    VipsImageInfo* childinfo = child->image->info;
    //printf("un-referencing child %d, info=%p valid=%d refcount=%d\n", i, childinfo, childinfo->valid, childinfo->refcount);
    if( childinfo && childinfo->valid ) {
      g_assert( childinfo->refcount>0 );
      childinfo->refcount -= 1;
      if( childinfo->refcount == 0 ) {
        free( childinfo->buf );
        childinfo->buf = NULL;
        childinfo->valid = 0;
        nbufactive -= 1;
        //printf("  clearing orphaned node info=%p  nbufactive=%d\n", childinfo, nbufactive);
      }
    }
  }

  // procedd also indirect children nodes
  for(int i = 0; i < node->nchildren_indirect; i++) {
    VipsImageTreeNode* child = node->children_indirect[i];
    VipsImageInfo* childinfo = child->image->info;
    //printf("un-referencing child %d, info=%p valid=%d refcount=%d\n", i, childinfo, childinfo->valid, childinfo->refcount);
    if( childinfo && childinfo->valid ) {
      g_assert( childinfo->refcount>0 );
      childinfo->refcount -= 1;
      if( childinfo->refcount == 0 ) {
        free( childinfo->buf );
        childinfo->buf = NULL;
        childinfo->valid = 0;
        nbufactive -= 1;
        //printf("  clearing indirect orphaned node info=%p  nbufactive=%d\n", childinfo, nbufactive);
      }
    }
  }

  return( 0 );
}


void* image_tree_sink_memory(VipsImageTree* tree, VipsRect* area)
{
  size_t bufsz = VIPS_IMAGE_SIZEOF_PEL(tree->root->image->image) * area->width * area->height;
  char* buf = (char*)malloc( bufsz );
  image_tree_sink_node( tree->root, area, 1, buf );
  return buf;
}
