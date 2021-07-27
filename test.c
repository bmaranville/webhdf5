
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  This program is an example that analyzes an HDF5 file, discovering
 *  all the objects and retreiving information about each.
 *
 *  There are many possibilities for how to scan the file and what
 *  to do with the objects.  This program just gives some simple
 *  illustrations.
 */

#include "hdf5.h"
#include <emscripten.h>

#define MAX_NAME 1024

EMSCRIPTEN_KEEPALIVE
hid_t* test(char *filename)
{

    hid_t    file;
    hid_t*    grp;

    herr_t   status;



       /*
        *  Example: open a file, open the root, scan the whole file.
        */
       file = H5Fopen(filename, H5F_ACC_RDWR, H5P_DEFAULT);
       printf("File ID: %llu\n", file);

       *grp = H5Gopen(file,"/", H5P_DEFAULT);
       //scan_group(grp);

       status = H5Fclose(file);

       return grp;
}

EMSCRIPTEN_KEEPALIVE
void open_hdf5(char *filename, hid_t* handle)
{

    /*
     *  Example: open a file, open the root, scan the whole file.
     */
       *handle = H5Fopen(filename, H5F_ACC_RDWR, H5P_DEFAULT);
       printf("File ID: %llu\n", *handle);
       return;
}

EMSCRIPTEN_KEEPALIVE
int64_t open_hdf5_bigint(char *filename)
{
    hid_t handle;
    /*
     *  Example: open a file, open the root, scan the whole file.
     */
    handle = H5Fopen(filename, H5F_ACC_RDWR, H5P_DEFAULT);
    //printf("File ID: %llu\n", handle);
    return (int64_t) handle;
}

EMSCRIPTEN_KEEPALIVE
unsigned int has_creation_order(hid_t file_id, char* group_name) {
    hid_t grp;
    hid_t plist;
    hid_t status;
    herr_t err;
    unsigned int ordered;

    grp = H5Gopen2(file_id, group_name, H5P_DEFAULT);
    plist = H5Gget_create_plist(grp);
    err = H5Pget_link_creation_order(plist, &ordered);
    ordered = (ordered & (unsigned int)H5P_CRT_ORDER_INDEXED);
    status = H5Gclose(grp);
    return ordered;
}

//EMSCRIPTEN_KEEPALIVE


EMSCRIPTEN_KEEPALIVE
hsize_t num_keys(hid_t file_id, char* group_name)
{
    hid_t grp;
    H5G_info_t info;
    herr_t err;
    hid_t status;

    grp = H5Gopen2(file_id, group_name, H5P_DEFAULT);
    err = H5Gget_info(grp, &info);
    status = H5Gclose(grp);
    return info.nlinks;
}

EMSCRIPTEN_KEEPALIVE
size_t get_link_name_length(hid_t file_id, char* group_name, hsize_t idx) {
    size_t name_length;

    // name_length = H5Lget_name_by_idx(file_id, group_name, H5_INDEX_CRT_ORDER, H5_ITER_INC, idx, NULL, NULL, H5P_DEFAULT);
    name_length = H5Lget_name_by_idx(file_id, group_name, H5_INDEX_NAME, H5_ITER_INC, idx, NULL, name_length, H5P_DEFAULT);

    return name_length;
}

EMSCRIPTEN_KEEPALIVE
void get_link_name(hid_t file_id, char* group_name, hsize_t idx, size_t name_length, char* name) 
{
    name_length = H5Lget_name_by_idx(file_id, group_name, H5_INDEX_NAME, H5_ITER_INC, idx, name, name_length, H5P_DEFAULT);
}

