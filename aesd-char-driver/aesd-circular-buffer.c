/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#include <linux/init.h>
#include <linux/module.h>
#else
#include <string.h>
#include <stdio.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer. 
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry 
*aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
			size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */ 

   struct  aesd_buffer_entry temp;
   uint8_t len = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
   int i;
   uint8_t index = 0;

    // calculate  length of circular buffer
    // uint8_t  len = ((buffer->in_offs - buffer->out_offs) & (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1));

    
   // if( !len ) len = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; //full 
   if((!buffer->full) && (buffer->in_offs == buffer->out_offs)) return NULL; //if empty

    index = buffer->out_offs;
   //start parsing at out_offs 
   
   for(i = 0; i < len; i++)
   {
       
    temp = buffer->entry[index];

    if( char_offset >= temp.size )
    {
        // update offset
        char_offset = char_offset - temp.size;
    }
    else
    {
        *entry_offset_byte_rtn = (char_offset);

        // printf("Returning string %s \n",temp.buffptr);
        // printf("Returning ofsetted string %s\n", &(buffer->entry[index].buffptr[*entry_offset_byte_rtn]));
        
        return &(buffer->entry[index]);

    }
    index++;

    //handle roll over for index
    if(index>=AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) 
        index=0;

   }
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.A
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
struct aesd_buffer_entry 
aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description 
    */
   struct aesd_buffer_entry ret = {NULL, 0};

    //check if full flag -> increment out_off
   if( buffer->full ) 
    {
        ret = buffer->entry[buffer->out_offs];
        buffer->out_offs++;
    }

    buffer->entry[buffer->in_offs] = *add_entry;   
     
   // increment in_off
   buffer->in_offs++;

   if(buffer->in_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) 
        buffer->in_offs = 0;

    if(buffer->out_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) 
        buffer->out_offs = 0;       // roll back as per circular buffer

   // if in_off == out_off -> set full flag
   if(buffer->in_offs == buffer->out_offs) buffer->full = true;

    return ret;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));

    buffer->in_offs = 0;
    buffer->out_offs = 0;
    buffer->full = false;

}

/*
*   Print entire queue
*
*/
void print_buffer(struct aesd_circular_buffer *buffer){

    struct  aesd_buffer_entry temp;
    int i;
    printk(KERN_ALERT "\n\n PRINT DEBUG \n\n");

    
    for( i=0; i< AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++)
    {
        temp = buffer->entry[i];
        
        printk(KERN_ALERT "%p: %s\n", temp.buffptr, temp.buffptr);
    }
}