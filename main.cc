#include<iostream>
#include <stdio.h>
#include <vips.h>


int main(int argc, char** argv)
{
  char* fullpath;

  if( argc > 1 ) {
    fullpath = realpath( argv[argc-1], NULL );
    if(!fullpath)
      return 1;

    VipsImage* img = vips_image_new_from_file( fullpath, NULL );
    printf("fullpath: %s  img: %p\n", fullpath, img);
    size_t array_sz;
    void* mem_array = vips_image_write_to_memory( img, &array_sz );
    printf("mem_array(1): %p\n", mem_array);
    if( mem_array ) free(mem_array);
    VipsImage* out;
    if( !vips_gaussblur( img, &out, 20, NULL ) ) {
      mem_array = vips_image_write_to_memory( out, &array_sz );
      printf("mem_array(2): %p\n", mem_array);
      if( mem_array ) free(mem_array);
    }
    if( !vips_subsample( img, &out, 2, 2, NULL ) ) {
      mem_array = vips_image_write_to_memory( out, &array_sz );
      printf("mem_array(3): %p\n", mem_array);
      if( mem_array ) free(mem_array);
    }
    if( !vips_resize( img, &out, 0.5, NULL) ) {
      mem_array = vips_image_write_to_memory( out, &array_sz );
      printf("mem_array(4): %p\n", mem_array);
      if( mem_array ) free(mem_array);
    }
  }
  return 0;
}
