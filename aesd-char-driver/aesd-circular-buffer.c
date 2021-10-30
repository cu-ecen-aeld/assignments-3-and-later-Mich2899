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
#else
#include <string.h>
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
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
			size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */

    int pos = buffer->out_offs;                                  //set the position to the read pointer of the circular buffer
    size_t nextsize = (buffer->entry[buffer->out_offs]).size;        //next size adds to the size of the next entry if the char offset is not found in the current one
    size_t prevsize = 0;                                             //prev size adds to the size of the previous entry to nextsize so as to subtract and find the byte position we require


    //return if buffer is NULL
    if(buffer == NULL)
        return NULL;

    //traverse in the loop until the we find the entry that has the char_offset byte
    //NOTE: We subtract 1 from the size of the entry as the index starts from 0
        while(char_offset > (nextsize-1)){

            prevsize += (buffer->entry[pos].size);                          //store the size of current entry
            pos = (pos+1) % (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED);      //increment the position and take the next entry

            if(pos == buffer->out_offs)                                     // check if the buffer is empty and return NULL if true
                return NULL;                                                   

            nextsize += (buffer->entry[pos].size);                          //take the size of next entry for checking with char_offset
        }

       *entry_offset_byte_rtn = (char_offset - prevsize);                   //the entry byte we require 
        return &(buffer->entry[pos]);                                       //return pointer to the current entry
        
    return NULL;
}

// /**
// * Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
// * If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
// * new start location.
// * Any necessary locking must be handled by the caller
// * Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
// */
// void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
// {
//     /**
//     * TODO: implement per description 
//     */

//     if(buffer == NULL || add_entry == NULL)         //return if buffer is NULL or the entry to be added is NULL
//     return;

//     if(buffer->in_offs == buffer->out_offs)         //check if the circular buffer is full 
//         buffer->full = true;

//     if(buffer->full){                               //if the buffer is full, advance buffer->out_offs to the new start location
//         buffer->out_offs = (buffer->out_offs+1) % (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED);
//     }

//     //add the entry to the ciruclar buffer
//     //NOTE: if the buffer was already full it will overwrite the entry and the read/ out offset is already set to the next pointer
//     buffer->entry[buffer->in_offs] = *add_entry;
//     buffer->in_offs = (buffer->in_offs+1) % (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED);

// }


/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* @return NULL or, if an exisiting entry at out_offs was replaced,
* the value of buffptr for the entry which was replaced (for use with dynamic memory allocation/free)
*/
const char *aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description 
    */

    const char *return_ptr=NULL;

    if(buffer == NULL || add_entry == NULL)         //return if buffer is NULL or the entry to be added is NULL
    return NULL;

    if(buffer->full){
        return_ptr = buffer->entry[buffer->out_offs].buffptr;
    }

    //add the entry to the ciruclar buffer
    //NOTE: if the buffer was already full it will overwrite the entry and the read/ out offset is already set to the next pointer
    buffer->entry[buffer->in_offs] = *add_entry;
    buffer->in_offs = (buffer->in_offs+1) % (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED);

    if(buffer->full){                               //if the buffer is full, advance buffer->out_offs to the new start location

        buffer->out_offs = (buffer->out_offs+1) % (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED);
    }

    if(buffer->in_offs == buffer->out_offs)         //check if the circular buffer is full 
        buffer->full = true;
    else 
        buffer->full = false;


    return return_ptr;


}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}

void aesd_circular_buffer_clean(struct aesd_circular_buffer *buffer)
{
    uint8_t index;
    struct aesd_buffer_entry *entry;

    AESD_CIRCULAR_BUFFER_FOREACH(entry, buffer, index)
    {
        if(entry->buffptr != NULL)
        {
#ifdef __KERNEL__
            kfree(entry->buffptr);
#else   
             free((char *)entry->buffptr);
#endif 
        }
    }
}