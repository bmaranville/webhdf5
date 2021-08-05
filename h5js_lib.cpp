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
#include <iostream>
#include <string>

#include <emscripten/bind.h>

using namespace emscripten;

#include "H5Cpp.h"
using namespace H5;

#define ATTRIBUTE_DATA 0
#define DATASET_DATA   1
#define ENUM_DATA      2
// Operator function

hsize_t obj_count(H5File file) {
    return file.getNumObjs();
}

herr_t name_callback(hid_t loc_id, const char *name, const H5L_info_t *linfo, void *opdata) {
    std::vector<std::string>* namelist = reinterpret_cast<std::vector<std::string> *>(opdata);
    (*namelist).push_back(name);
    return 0;
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
    grp.close();
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
        shape.set(d, (uint)dims_out[d]);
    }
    attr.set("shape", shape);
    attr.set("total_size", total_size);

    DataType dtype = ds.getDataType();
    size_t size = dtype.getSize();
    H5T_class_t dtype_class = dtype.getClass();
    H5T_order_t order;
    H5T_cset_t cset;
    H5T_sign_t is_signed = H5T_SGN_2;

    switch (dtype_class) {
        case H5T_STRING: {
            //StrType ts = attr_obj.getStrType();
            StrType str_type = ds.getStrType();
            order = str_type.getOrder();
            cset = str_type.getCset();
            //htri_t is_variable_len = H5Tis_variable_str(str_type.getId());
            attr.set("typestr", "string");
            attr.set("cset", (int) cset);
            str_type.close();
            break;
        }
        case H5T_INTEGER: 
        {
            IntType int_type = ds.getIntType();
            order = int_type.getOrder();
            is_signed = int_type.getSign();
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
    attr.set("vlen", dtype.isVariableStr());
    attr.set("signed", (bool)is_signed);
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

val get_data(AbstractDs & ds, int source, DataSpace* dspace, DataSpace* memspace) {
    
    //size_t datasize = ds.getInMemDataSize();
    DataType dtype = ds.getDataType();
    htri_t is_variable_len = H5Tis_variable_str(dtype.getId());
    int total_size = memspace->getSimpleExtentNpoints();
    // std::cout << total_size << std::endl;
    size_t size = dtype.getSize();
    H5T_class_t dtype_class = dtype.getClass();
    bool is_vlstr = dtype.isVariableStr();
    Attribute * attribute_obj;
    DataSet * dataset_obj;
    val output = val::undefined();
    
    if (source == ATTRIBUTE_DATA) {
        attribute_obj = (Attribute *) &ds;
    }
    else if (source == DATASET_DATA) {
        dataset_obj = (DataSet *) &ds;
    }

    if (dtype_class == H5T_STRING) 
    {
        int ndims = memspace->getSimpleExtentNdims();       
        if (ndims > 0) {
            output = val::array();

            void * buf = malloc(total_size*size);
            if (source == ATTRIBUTE_DATA) {
                attribute_obj->read(dtype, buf);
            }
            else if (source == DATASET_DATA) {
                dataset_obj->read(buf, dtype, *memspace, *dspace);
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
                    H5Treclaim(dtype.getId(), memspace->getId(), H5P_DEFAULT, buf);
                free(buf);
            }            
        }
        else {
            H5std_string readbuf;
            if (source == ATTRIBUTE_DATA) {
                attribute_obj->read(dtype, readbuf);
                attribute_obj->close();
            }
            else if (source == DATASET_DATA) {
                dataset_obj->read(readbuf, dtype, *memspace, *dspace);
                dataset_obj->close();
            }
            output = val(readbuf);
        }        
    }
    else 
    {
        thread_local const val Uint8Array = val::global("Uint8Array");
        uint8_t * buffer = (uint8_t*)malloc(size * total_size);
        
        if (source == ATTRIBUTE_DATA) {
            attribute_obj->read(dtype, buffer);
            attribute_obj->close();
        }
        else if (source == DATASET_DATA) {
            dataset_obj->read(buffer, dtype, *memspace, *dspace);
            dataset_obj->close();
        }
        
        output = Uint8Array.new_(typed_memory_view(
           total_size*size, buffer
        ));

        free(buffer);
    }
    
    dtype.close();
    dspace->close();
    memspace->close();
    
    return output;
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
    
    DataSpace dspace = attr_obj.getSpace();
    return get_data(attr_obj, ATTRIBUTE_DATA, &dspace, &dspace);
}

//val get_dataset_data(H5File file, H5std_string dataset_name, std::unique_ptr<hsize_t[]> count_out, std::unique_ptr<hsize_t[]> offset_out) {
val get_dataset_data(H5File file, H5std_string dataset_name, val count_out, val offset_out) {
    DataSet ds = file.openDataSet(dataset_name);
    DataSpace memspace;
    DataSpace dspace = ds.getSpace();
    if (count_out != val::null() && offset_out != val::null()) {
         std::vector<hsize_t> count = vecFromJSArray<hsize_t>(count_out);
         std::vector<hsize_t> offset = vecFromJSArray<hsize_t>(offset_out);
         memspace = *(new DataSpace(count.size(), &count[0]));
        //  std::cout << "count size: " << count.size() << std::endl;
        //  for (int i=0; i<count.size(); i++) {
        //    std::cout << i << ", " << count[i] << std::endl;
        //  }
        //  std::cout << "offset size: " << offset.size() << std::endl;
        //  for (int j=0; j<offset.size(); j++) {
        //    std::cout << j << ", " << offset[j] << std::endl;
        //  }
         dspace.selectHyperslab(H5S_SELECT_SET, &count[0], &offset[0]);
         memspace.selectAll();
    // if (count_out && offset_out) {
    //   memspace.selectHyperslab(H5S_SELECT_SET, (hsize_t *)count_out, (hsize_t *)offset_out);
    }
    else {
     dspace.selectAll();
     memspace = *(new DataSpace(dspace));
    }
    /*
          memspace.selectHyperslab( H5S_SELECT_SET, count_out, offset_out );
          std::vector<T> vecFromJSArray(const val &v)
    */
    return get_data(ds, DATASET_DATA, &dspace, &memspace);
}

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
      .function("close", &H5File::close)
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
