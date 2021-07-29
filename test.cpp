/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*
 * This example creates a group in the file and dataset in the group.
 * Hard link to the group object is created and the dataset is accessed
 * under different names.
 * Iterator function is used to find the object names in the root group.
 * Note that the C++ API iterator function is not completed yet, thus
 * the C version is used in this example.
 */
#ifdef OLD_HEADER_FILENAME
#include <iostream.h>
#else
#include <iostream>
#endif
using std::cout;
using std::endl;
#include <emscripten/bind.h>

using namespace emscripten;

#include <string>
#include "H5Cpp.h"
using namespace H5;
const H5std_string FILE_NAME( "Group.h5" );
const int      RANK = 2;
#define ATTRIBUTE_DATA 0
#define DATASET_DATA   1
#define ENUM_DATA      2
// Operator function
extern "C" herr_t file_info(hid_t loc_id, const char *name, const H5L_info_t *linfo,
    void *opdata);

extern "C" herr_t name_callback(hid_t loc_id, const char *name, const H5L_info_t *linfo,
    void *opdata);

int main(void)
{
    hsize_t  dims[2];
    hsize_t  cdims[2];
    // Try block to detect exceptions raised by any of the calls inside it
    try
    {
    /*
     * Turn off the auto-printing when failure occurs so that we can
     * handle the errors appropriately
     */
    Exception::dontPrint();
    /*
     * Create the named file, truncating the existing one if any,
     * using default create and access property lists.
     */
    H5File *file = new H5File( FILE_NAME, H5F_ACC_TRUNC );
    /*
     * Create a group in the file
     */
    Group* group = new Group( file->createGroup( "/Data" ));
    /*
     * Create dataset "Compressed Data" in the group using absolute
     * name. Dataset creation property list is modified to use
     * GZIP compression with the compression effort set to 6.
     * Note that compression can be used only when dataset is chunked.
     */
    dims[0] = 1000;
    dims[1] = 20;
    cdims[0] = 20;
    cdims[1] = 20;
    DataSpace *dataspace = new DataSpace(RANK, dims); // create new dspace
    DSetCreatPropList ds_creatplist;  // create dataset creation prop list
    ds_creatplist.setChunk( 2, cdims );  // then modify it for compression
    ds_creatplist.setDeflate( 6 );
    /*
     * Create the first dataset.
     */
    DataSet* dataset = new DataSet(file->createDataSet(
        "/Data/Compressed_Data", PredType::NATIVE_INT,
        *dataspace, ds_creatplist ));
    /*
     * Close the first dataset.
     */
    delete dataset;
    delete dataspace;
    /*
     * Create the second dataset.
     */
    dims[0] = 500;
    dims[1] = 20;
    dataspace = new DataSpace(RANK, dims); // create second dspace
    dataset = new DataSet(file->createDataSet("/Data/Float_Data",
            PredType::NATIVE_FLOAT, *dataspace));
    delete dataset;
    delete dataspace;
    delete group;
    delete file;
    /*
     * Now reopen the file and group in the file.
     */
    file = new H5File(FILE_NAME, H5F_ACC_RDWR);
    group = new Group(file->openGroup("Data"));
    /*
     * Access "Compressed_Data" dataset in the group.
     */
    try {  // to determine if the dataset exists in the group
        dataset = new DataSet( group->openDataSet( "Compressed_Data" ));
    }
    catch( GroupIException not_found_error ) {
        cout << " Dataset is not found." << endl;
    }
    cout << "dataset \"/Data/Compressed_Data\" is open" << endl;
    /*
     * Close the dataset.
     */
    delete dataset;
    /*
     * Create hard link to the Data group.
     */
    file->link( H5L_TYPE_HARD, "Data", "Data_new" );
    /*
     * We can access "Compressed_Data" dataset using created
     * hard link "Data_new".
     */
    try {  // to determine if the dataset exists in the file
        dataset = new DataSet(file->openDataSet( "/Data_new/Compressed_Data" ));
    }
    catch( FileIException not_found_error )
    {
        cout << " Dataset is not found." << endl;
    }
    cout << "dataset \"/Data_new/Compressed_Data\" is open" << endl;
    /*
     * Close the dataset.
     */
    delete dataset;
    /*
     * Use iterator to see the names of the objects in the file
     * root directory.
     */
    cout << endl << "Iterating over elements in the file" << endl;
    herr_t idx = H5Literate(file->getId(), H5_INDEX_NAME, H5_ITER_INC, NULL, file_info, NULL);
    cout << endl;
    /*
     * Unlink  name "Data" and use iterator to see the names
     * of the objects in the file root direvtory.
     */
    cout << "Unlinking..." << endl;
    try {  // attempt to unlink the dataset
        file->unlink( "Data" );
    }
    catch( FileIException unlink_error )
    {
        cout << " unlink failed." << endl;
    }
    cout << "\"Data\" is unlinked" << endl;
    cout << endl << "Iterating over elements in the file again" << endl;
    idx = H5Literate(file->getId(), H5_INDEX_NAME, H5_ITER_INC, NULL, file_info, NULL);
    cout << endl;
    /*
     * Close the group and file.
     */
    delete group;
    delete file;
    }  // end of try block
    // catch failure caused by the H5File operations
    catch( FileIException error )
    {
    error.printErrorStack();
    return -1;
    }
    // catch failure caused by the DataSet operations
    catch( DataSetIException error )
    {
    error.printErrorStack();
    return -1;
    }
    // catch failure caused by the DataSpace operations
    catch( DataSpaceIException error )
    {
    error.printErrorStack();
    return -1;
    }
    // catch failure caused by the Attribute operations
    catch( AttributeIException error )
    {
    error.printErrorStack();
    return -1;
    }
    return 0;
}
/*
 * Operator function.
 */
herr_t
file_info(hid_t loc_id, const char *name, const H5L_info_t *linfo, void *opdata)
{
    hid_t group;
    /*
     * Open the group using its name.
     */
    group = H5Gopen2(loc_id, name, H5P_DEFAULT);
    /*
     * Display group name.
     */
    cout << "Name : " << name << endl;
    H5Gclose(group);
    return 0;
}


float lerp(float a, float b, float t) {
    return (1 - t) * a + t * b;
}

/*
uint64 open_h5(const char *name) {
    H5File* file = new H5File(name, H5F_ACC_RDONLY);
    return file;
}
*/

hsize_t obj_count(H5File file) {
    return file.getNumObjs();
}

std::vector<std::string> get_keys_vector(hid_t group_id) {
    //val output = val::array();
    std::vector<std::string> namelist;
    herr_t idx = H5Literate(group_id, H5_INDEX_NAME, H5_ITER_INC, NULL, name_callback, &namelist);
    return namelist;
}

val get_keys(hid_t group_id) {
    //val output = val::array();
    std::vector<std::string> namelist = get_keys_vector(group_id);
    return val::array(namelist);
}

val get_link_types(hid_t group_id) {
    H5L_info2_t info;
    val output = val::array();
    std::vector<std::string> keys = get_keys_vector(group_id);
    size_t index = 0; 
    for(std::vector<std::string>::iterator it = keys.begin(); it != keys.end(); ++it) {
        H5Lget_info(group_id, it->c_str(), &info, H5P_DEFAULT);
        output.set(index++, (int)(info.type));
    }
    return output;
}

val get_types(hid_t group_id) {
    H5O_info_t info;
    val output = val::array();
    std::vector<std::string> keys = get_keys_vector(group_id);
    size_t index = 0; 
    for(std::vector<std::string>::iterator it = keys.begin(); it != keys.end(); ++it) {
        H5Oget_info_by_name3(group_id, it->c_str(), &info, H5O_INFO_ALL, H5P_DEFAULT);
        output.set(index++, (int)(info.type));
    }
    return output;
}

herr_t name_callback(hid_t loc_id, const char *name, const H5L_info_t *linfo, void *opdata) {
    std::vector<std::string>* namelist = reinterpret_cast<std::vector<std::string> *>(opdata);
    (*namelist).push_back(name);
    return 0;
}

size_t MAX_NAME = 1024;

std::string get_link_name(H5File file, std::string group_name, hsize_t idx) 
{
    hid_t grp_id = file.getObjId(group_name);
    Group grp = Group(grp_id);
    std::string name;
    name = grp.getObjnameByIdx(idx);
    grp.close();
    return name;
}

val get_names2(H5File file, std::string group_name) {
    val names = val::array();
    Group grp = Group(file.getObjId(group_name));
    std::string name;
    hsize_t numObjs = grp.getNumObjs();
    for (hsize_t i = 0; i < numObjs; i++) {
      name = grp.getObjnameByIdx(i);
      names.set(i, name);
    }
    return names;
}

val get_types2(H5File file, std::string group_name) {
    val types = val::array();
    Group grp = Group(file.getObjId(group_name));
    H5O_type_t type;
    std::string name;
    hsize_t numObjs = grp.getNumObjs();
    for (hsize_t i = 0; i < numObjs; i++) {
      name = grp.getObjnameByIdx(i);
      type = grp.childObjType(name);
      types.set(i, (int) type);
    }
    grp.close();
    return types;
}

int get_type(H5File file, std::string obj_name) {
    return (int) file.childObjType(obj_name);
}

val get_shape(DataSpace& dspace) {
    int rank = dspace.getSimpleExtentNdims();
    hsize_t dims_out[rank];
    int ndims = dspace.getSimpleExtentDims( dims_out, NULL);
    val shape = val::array();
    for (int d=0; d<ndims; d++) {
        shape.set(d, dims_out[d]);
    }
    return shape;
}

val get_abstractds_metadata(AbstractDs& ds) {    
    val attr = val::object();
    DataSpace dspace = ds.getSpace();
    int rank = dspace.getSimpleExtentNdims();
    int total_size = dspace.getSimpleExtentNpoints();
    hsize_t dims_out[rank];
    int ndims = dspace.getSimpleExtentDims( dims_out, NULL);
    val shape = val::array();
    for (int d=0; d<ndims; d++) {
        shape.set(d, dims_out[d]);
    }
    attr.set("shape", shape);
    attr.set("total_size", total_size);

    DataType dtype = ds.getDataType();
    size_t size = dtype.getSize();
    H5T_class_t dtype_class = dtype.getClass();
    H5T_order_t order;
    H5T_cset_t cset;

    switch (dtype_class) {
        case H5T_STRING: {
            //StrType ts = attr_obj.getStrType();
            StrType str_type = ds.getStrType();
            order = str_type.getOrder();
            cset = str_type.getCset();
            //htri_t is_variable_len = H5Tis_variable_str(str_type.getId());
            attr.set("vlen", str_type.isVariableStr());
            attr.set("typestr", "string");
            attr.set("cset", (int) cset);
            str_type.close();
            break;
        }
        case H5T_INTEGER: 
        {
            IntType int_type = ds.getIntType();
            order = int_type.getOrder();
            H5T_sign_t is_signed = int_type.getSign();
            attr.set("signed", (bool)is_signed);
            attr.set("typestr", "int");
            int_type.close();
            break;
        }
        case H5T_FLOAT: {
            FloatType float_type = ds.getFloatType();
            order = float_type.getOrder();
            attr.set("typestr", "float");
            float_type.close();
            break;
        }

        default:
            // pass
            break;
    }
    bool littleEndian = (order == H5T_ORDER_LE);
    attr.set("littleEndian", littleEndian);
    attr.set("type", (int) dtype_class);
    attr.set("size", size);
    attr.set("memsize", ds.getInMemDataSize());
    
    dspace.close();
    dtype.close();
    return attr;
}

val get_dataset_metadata(H5File file, std::string dataset_name) {
    DataSet d = file.openDataSet(dataset_name);
    val metadata = get_abstractds_metadata(d);
    d.close();
    return metadata;
}

void object_get_attr_metadata(H5Object& loc, H5std_string name, void* opdata) {
    //std::vector<std::string>* namelist = reinterpret_cast<std::vector<std::string> *>(opdata);
    val *attrs = reinterpret_cast<val *>(opdata);
    Attribute attr_obj = loc.openAttribute(name);
    val attr = get_abstractds_metadata(attr_obj);
    attr.set("id", attr_obj.getId());
    attr_obj.close();

    attrs->set(name, attr);
}

H5std_string read_attribute_string(Attribute attr_obj, int offset) {
    return "";
}

val get_attribute_metadata(H5File file, H5std_string obj_name, H5std_string attr_name) {
    H5O_type_t type = file.childObjType(obj_name);
    Attribute attr_obj;
    switch(type) {
        case H5O_TYPE_DATASET: 
        {
            attr_obj = file.openDataSet(obj_name).openAttribute(attr_name);
            break;
        }
        case H5O_TYPE_GROUP: 
        {
            attr_obj = file.openGroup(obj_name).openAttribute(attr_name);
            break;
        }
        case H5O_TYPE_MAP:
        {
            attr_obj = file.openDataType(obj_name).openAttribute(attr_name);
            break;
        }
        default:
            break;

    }

    val attr = get_abstractds_metadata(attr_obj);
    attr.set("id", attr_obj.getId());
    attr_obj.close();
    return attr;
}

val get_data(AbstractDs & ds, int source) {
    
    size_t datasize = ds.getInMemDataSize();
    DataType dtype = ds.getDataType();
    htri_t is_variable_len = H5Tis_variable_str(dtype.getId());

    DataSpace dspace = ds.getSpace();
    int total_size = dspace.getSimpleExtentNpoints();
    size_t size = dtype.getSize();
    H5T_class_t dtype_class = dtype.getClass();
    bool is_vlstr = dtype.isVariableStr();
    Attribute * attribute_obj;
    DataSet * dataset_obj;
    
    if (source == ATTRIBUTE_DATA) {
        attribute_obj = (Attribute *) &ds;
    }
    else if (source == DATASET_DATA) {
        dataset_obj = (DataSet *) &ds;
    }

    if (dtype_class == H5T_STRING) 
    {
        StrType str_type = ds.getStrType();
        int ndims = dspace.getSimpleExtentNdims();
        
        if (ndims > 0) {
            val output = val::array();

            void * buf = malloc(total_size*size);
            if (source == ATTRIBUTE_DATA) {
                attribute_obj->read(dtype, buf);
            }
            else if (source == DATASET_DATA) {
                dataset_obj->read(buf, dtype);
            }
            char * bp = (char *) buf;
            char * onestring = NULL;
            if (!is_vlstr)
                onestring = (char *)calloc(size, sizeof(char));

            for (int i=0; i<total_size; i++) {
                if (is_vlstr) {
                    onestring = *(char **)((void *)bp);
                }
                else {
                    strncpy(onestring, bp, size);
                }
                output.set(i, val(std::string(onestring)));
                bp += size;
            }
            
            if (!is_vlstr)
                if (onestring)
                    free(onestring);

            if (buf) {
                if (is_vlstr)
                    H5Treclaim(dtype.getId(), dspace.getId(), H5P_DEFAULT, buf);
                free(buf);
            }
            return output;
            
        }
        else {
            H5std_string readbuf;
            if (source == ATTRIBUTE_DATA) {
                attribute_obj->read(dtype, readbuf);
                attribute_obj->close();
            }
            else if (source == DATASET_DATA) {
                dataset_obj->read(readbuf, dtype);
                dataset_obj->close();
            }
            //cout << readbuff.length() << " length" << endl;
            dtype.close();
            str_type.close();
            dspace.close();
            return val(readbuf);
        }
        
    }
    else 
    {
        //std::vector<uint8_t> vec (datasize);
        //const uint8_t * buffer = vec.data();
        thread_local const val Uint8Array = val::global("Uint8Array");
        uint8_t * buffer = (uint8_t*)malloc(size * total_size);
        
        if (source == ATTRIBUTE_DATA) {
            attribute_obj->read(dtype, buffer);
            attribute_obj->close();
        }
        else if (source == DATASET_DATA) {
            dataset_obj->read(buffer, dtype);
            dataset_obj->close();
        }
        
        val output = Uint8Array.new_(typed_memory_view(
           datasize, buffer
        ));
        //val output = val(typed_memory_view(datasize, buffer));

        free(buffer);
        dtype.close();
        dspace.close();
        return output;

    }
}

val get_attribute_data(H5File file, H5std_string obj_name, H5std_string attr_name) {
    //H5File file = H5File(file_id);
    H5O_type_t type = file.childObjType(obj_name);
    //size_t datasize;
    //DataType dtype;
    Attribute attr_obj;
    switch(type) {
        case H5O_TYPE_DATASET: 
        {
            attr_obj = file.openDataSet(obj_name).openAttribute(attr_name);
            break;
        }
        case H5O_TYPE_GROUP: 
        {
            attr_obj = file.openGroup(obj_name).openAttribute(attr_name);
            break;
        }
        case H5O_TYPE_MAP:
        {
            attr_obj = file.openDataType(obj_name).openAttribute(attr_name);
            break;
        }
        default:
            break;

    }
    
    return get_data(attr_obj, ATTRIBUTE_DATA);
}

val get_dataset_data(H5File file, H5std_string dataset_name) {
    DataSet ds = file.openDataSet(dataset_name);
    return get_data(ds, DATASET_DATA);
}

/*
    size_t datasize = attr_obj.getInMemDataSize();
    DataType dtype = attr_obj.getDataType();
    htri_t is_variable_len = H5Tis_variable_str(dtype.getId());

    DataSpace dspace = attr_obj.getSpace();
    int total_size = dspace.getSimpleExtentNpoints();
    size_t size = dtype.getSize();
    H5T_class_t dtype_class = dtype.getClass();
    bool is_vlstr = dtype.isVariableStr();

    if (dtype_class == H5T_STRING) 
    {
        StrType str_type = attr_obj.getStrType();
        int ndims = dspace.getSimpleExtentNdims();
        IntType int_type = attr_obj.getIntType();
        //IntType int_type = IntType();
        // int_type.setSign((H5T_sign_t)0);
        // int_type.setSize((size_t)1);
        // int_type.setOrder((H5T_order_t)0);
        if (ndims > 0) {
            val output = val::array();

            void * buf = malloc(total_size*size);
            attr_obj.read(dtype, buf);
            char * bp = (char *) buf;
            char * onestring = NULL;
            if (!is_vlstr)
                onestring = (char *)calloc(size, sizeof(char));

            for (int i=0; i<total_size; i++) {
                if (is_vlstr) {
                    onestring = *(char **)((void *)bp);
                }
                else {
                    strncpy(onestring, bp, size);
                }
                output.set(i, val(std::string(onestring)));
                bp += size;
            }
            //uint8_t * buffer = (uint8_t*)malloc(total_size);
            //attr_obj.read(int_type, buffer);
            //val output = Uint8Array.new_(typed_memory_view(
            //    datasize, buffer
            //));
            //val output = val(typed_memory_view(datasize, buffer));

            //free(buffer);
            //for (int i=0; i<total_size; i++) {
            //    H5std_string readbuff;
            //    attr_obj.read(str_type, readbuff);
                //herr_t err = H5Treclaim(str_type.getId(), dspace.getId(), H5P_DEFAULT, &readbuff);
            //    output.set(i, val(readbuff));
            //}
            //H5Treclaim(str_type.getId(), dspace.getId(), H5P_DEFAULT, buffer);
            if (!is_vlstr)
                if (onestring)
                    free(onestring);

            if (buf) {
                if (is_vlstr)
                    H5Treclaim(dtype.getId(), dspace.getId(), H5P_DEFAULT, buf);
                free(buf);
            }
            return output;
            
        }
        else {
            H5std_string readbuff;
            attr_obj.read(str_type, readbuff);
            //cout << readbuff.length() << " length" << endl;
            attr_obj.close();
            dtype.close();
            str_type.close();
            dspace.close();
            return val(readbuff);
        }
        
    }
    else 
    {
        //std::vector<uint8_t> vec (datasize);
        //const uint8_t * buffer = vec.data();
        thread_local const val Uint8Array = val::global("Uint8Array");
        uint8_t * buffer = (uint8_t*)malloc(size * total_size);
        //void * buffer = malloc(size * total_size);
        // new uint8_t[datasize];
        //H5std_string readbuff;
        //attr_obj.read(dtype, readbuff);
        //const char* buffer = readbuff.c_str();
        //printf("buffer address: %d\n", (int)buffer);
        //char* buffer = new char[datasize];
        attr_obj.read(dtype, buffer);
        
        // for (int i=0; i<datasize; i++) {
        //     cout << (uint)buffer[i] << ",";
        // }
        //cout << endl;
        val output = Uint8Array.new_(typed_memory_view(
           datasize, buffer
        ));
        //val output = val(typed_memory_view(datasize, buffer));

        free(buffer);
        attr_obj.close();
        dtype.close();
        dspace.close();
        return output;

    }
}

*/

/*
IntType int_type = attr_obj.getIntType();
        H5T_sign_t is_signed = int_type.getSign();
        if (size == 1 && is_signed) {
            std::vector<int8_t> vec (total_size, 0);
            int8_t * buffer = vec.data();
            attr_obj.read(dtype, buffer);
            return val(typed_memory_view(total_size, buffer));
        }
        else if (size == 1 && !is_signed) {
            std::vector<uint8_t> vec (total_size, 0);
            uint8_t * buffer = vec.data();
            attr_obj.read(dtype, buffer);
            return val(typed_memory_view(total_size, buffer));
        }
        else if (size == 2 && is_signed) {
            std::vector<int16_t> vec (total_size, 0);
            int16_t * buffer = vec.data();
            attr_obj.read(dtype, buffer);
            return val(typed_memory_view(total_size, buffer));
        }
        else if (size == 2 && !is_signed) {
            std::vector<uint16_t> vec (total_size, 0);
            uint16_t * buffer = vec.data();
            attr_obj.read(dtype, buffer);
            return val(typed_memory_view(total_size, buffer));
        }
        else if (size == 2 && is_signed) {
            std::vector<int16_t> vec (total_size, 0);
            int16_t * buffer = vec.data();
            attr_obj.read(dtype, buffer);
            return val(typed_memory_view(total_size, buffer));
        }
        else if (size == 2 && !is_signed) {
            std::vector<uint16_t> vec (total_size, 0);
            uint16_t * buffer = vec.data();
            attr_obj.read(dtype, buffer);
            return val(typed_memory_view(total_size, buffer));
        }
        else if (size == 4 && is_signed) {
            std::vector<int32_t> vec (total_size, 0);
            int32_t * buffer = vec.data();
            attr_obj.read(dtype, buffer);
            return val(typed_memory_view(total_size, buffer));
        }
        else if (size == 4 && !is_signed) {
            std::vector<uint32_t> vec (total_size, 0);
            uint32_t * buffer = vec.data();
            attr_obj.read(dtype, buffer);
            return val(typed_memory_view(total_size, buffer));
        }
        else if (size == 8 && is_signed) {
            std::vector<int64_t> vec (total_size, 0);
            int64_t * buffer = vec.data();
            attr_obj.read(dtype, buffer);
            return val(typed_memory_view(total_size, buffer));
        }
        else if (size == 8 && !is_signed) {
            std::vector<uint64_t> vec (total_size, 0);
            uint64_t * buffer = vec.data();
            attr_obj.read(dtype, buffer);
            return val(typed_memory_view(total_size, buffer));
        }
*/

// val get_dataset_data(H5File file, H5std_string dataset_name) {
//     DataSet ds = file.openDataSet(dataset_name);
//     val data = get_data(ds);
//     ds.close();
//     return data;
// }

val get_attrs_metadata(H5File file, std::string obj_name) {
    hid_t obj_id = file.getObjId(obj_name);
    H5O_type_t type = file.childObjType(obj_name);
    val attrs = val::object();
    val attr_names = val::array();
    unsigned idx = 0;
    switch(type) {
        case H5O_TYPE_DATASET: 
        {
            DataSet o = file.openDataSet(obj_name);
            o.iterateAttrs(object_get_attr_metadata, &idx, &attrs);
            o.close();
            break;
        }
        case H5O_TYPE_GROUP: 
        {
            Group o = file.openGroup(obj_name);
            o.iterateAttrs(object_get_attr_metadata, &idx, &attrs);
            o.close();
            break;
        }
        case H5O_TYPE_MAP:
        {
            DataType o = file.openDataType(obj_name);
            o.iterateAttrs(object_get_attr_metadata, &idx, &attrs);
            o.close();
        }
        default:
            break;

    }
    return attrs;
}



// val get_type(H5File file, std::string group_name, hsize_t idx) {
//     Group grp = Group(file.getObjId(group_name));
//     return val(grp.getObjTypeByIdx(idx));
// }
 
EMSCRIPTEN_BINDINGS(my_module) {
    function("lerp", &lerp);
    function("main", &main);
    function("obj_count", &obj_count);
    function("get_keys", &get_keys);
    function("get_link_name", &get_link_name);
    function("get_names2", &get_names2);
    function("get_link_types", &get_link_types);
    function("get_types", &get_types);
    function("get_types2", &get_types2);
    function("get_type", &get_type);
    function("get_attrs_metadata", &get_attrs_metadata);
    function("get_dataset_metadata", &get_dataset_metadata);
    function("get_attribute_data", &get_attribute_data);
    function("get_attribute_metadata", &get_attribute_metadata);
    function("get_dataset_data", &get_dataset_data);
    class_<H5File>("H5File")
      .constructor<std::string, int>()
      .function("getId", &H5File::getId)
      .function("getNumObjs", &H5File::getNumObjs)
      .function("get_types", &get_types2)
      ;
    class_<H5L_info2_t>("H5L_info2_t")
      .constructor<>()
      .property("type", &H5L_info2_t::type)
      .property("corder_valid", &H5L_info2_t::corder_valid)
      .property("corder", &H5L_info2_t::corder)
      .property("cset", &H5L_info2_t::cset)
      .property("u", &H5L_info2_t::u)
    ;
    enum_<H5L_type_t>("H5L_type_t")
      .value("H5L_TYPE_ERROR", H5L_type_t::H5L_TYPE_ERROR)
      .value("H5L_TYPE_HARD", H5L_type_t::H5L_TYPE_HARD)
      .value("H5L_TYPE_SOFT", H5L_type_t::H5L_TYPE_SOFT)
      .value("H5L_TYPE_EXTERNAL", H5L_type_t::H5L_TYPE_EXTERNAL)
      .value("H5L_TYPE_MAX", H5L_type_t::H5L_TYPE_MAX)
    ;
    class_<Group>("H5Group");
    //constant("H5L_type_t", H5L_type_t);
    // FILE ACCESS
    constant("H5F_ACC_RDONLY", H5F_ACC_RDONLY);
    constant("H5F_ACC_RDWR", H5F_ACC_RDWR);
    constant("H5F_ACC_TRUNC", H5F_ACC_TRUNC);
    constant("H5F_ACC_EXCL", H5F_ACC_EXCL);
    constant("H5F_ACC_CREAT", H5F_ACC_CREAT);
    constant("H5F_ACC_SWMR_WRITE", H5F_ACC_SWMR_WRITE);
    constant("H5F_ACC_SWMR_READ", H5F_ACC_SWMR_READ);
    
    constant("H5P_DEFAULT", H5P_DEFAULT);
    constant("H5O_TYPE_GROUP", (int)H5O_TYPE_GROUP);
    constant("H5O_TYPE_DATASET", (int)H5O_TYPE_DATASET);
    constant("H5O_TYPE_NAMED_DATATYPE", (int)H5O_TYPE_NAMED_DATATYPE);

    register_vector<std::string>("vector<string>");
}
    //function("open_h5", &open_h5, allow_raw_pointers());
//}
