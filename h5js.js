import ModuleFactory from './webhdf5.js';

var webhdf5 = null;
export { webhdf5 };

ModuleFactory().then((result) => { webhdf5 = result });

// const Module = ModuleFactory({
//     locateFile: function(path, prefix) {
//         // if it's a mem init file, use a custom dir
//         console.log(path, prefix);
//         if (path.endsWith(".wasm")) return "./" + path;
//         // otherwise, use the default, the prefix (JS file's dir) + the path

//         return prefix + path;
//     }
// });


//export {Module};

export function upload_file() {
    console.log(this);
    console.log(arguments);
    let file = this.files[0]; // only one file allowed
    let datafilename = file.name;
    let reader = new FileReader();
    reader.onloadend = function (evt) {
        let data = evt.target.result;
        webhdf5.FS.writeFile(datafilename, new Uint8Array(data));
        // Call WebAssembly/C++ to process the file
        //console.log(Module.processFile(filename));
        console.log('file loaded', datafilename);
    }
    reader.readAsArrayBuffer(file);

    this.value = "";

}


const ACCESS_MODES = {
    "r": "H5F_ACC_RDONLY",
    "a": "H5F_ACC_RDWR",
    "w": "H5F_ACC_TRUNC",
    "x": "H5F_ACC_EXCL",
    "c": "H5F_ACC_CREAT",
    "Sw": "H5F_ACC_SWMR_WRITE",
    "Sr": "H5F_ACC_SWMR_READ"
}

function normalizePath(path) {
    if (path == "/") { return path }
    // replace multiple path separators with single
    path = path.replace(/\/(\/)+/g, '/');
    // strip end slashes
    path = path.replace(/(\/)+$/, '');
    return path;
}

class Attribute {
    constructor(file_obj, path, name) {
        this._file_obj = file_obj;
        this._path = path;
        this._name = name;
    }

    get value() {
        return get_attr(this._file_obj, this._path, this._name);
    } 
}

function get_attr(file_obj, obj_name, attr_name) {
    //let attrs_metadata = webhdf5.get_attrs_metadata(this.file_obj, obj_name);
    //let metadata = attrs_metadata[attr_name];
    let metadata = webhdf5.get_attribute_metadata(file_obj, obj_name, attr_name);
    let data = webhdf5.get_attribute_data(file_obj, obj_name, attr_name);
    return process_data(data, metadata)
}

function process_data(data, metadata) {
    if (metadata.typestr == "string") {
        return data;
    }
    else if (metadata.typestr == "int") {
        let accessor = (metadata.signed) ? "Int" : "Uint";
        accessor += ((metadata.size) * 8).toFixed() + "Array";
        return new globalThis[accessor](data.buffer);

    }
    else if (metadata.typestr == "float") {
        let accessor = "Float" + ((metadata.size) * 8).toFixed() + "Array";
        return new globalThis[accessor](data.buffer);
    }
}

class HasAttrs {
    get attrs() {
        let attrs_metadata = webhdf5.get_attrs_metadata(this.file_obj, this.path);
        //let attrs = Object.fromEntries(Object.keys(attrs_metadata).map(k => [k, new Attribute(this.file_obj, this.path, k)]));
        //return attrs;
        let attrs = {};
        for (let [name, metadata] of Object.entries(attrs_metadata)) {
            Object.defineProperty(attrs, name, {
                get: () => ({
                    value: get_attr(this.file_obj, this.path, name),
                    metadata
                }),
                enumerable: true
            });
        }
        return attrs;
        
    }

    // get_attr(attr_name) {
    //     //let attrs_metadata = webhdf5.get_attrs_metadata(this.file_obj, obj_name);
    //     //let metadata = attrs_metadata[attr_name];
    //     let obj_name = this.path;
    //     let metadata = webhdf5.get_attribute_metadata(this.file_obj, obj_name, attr_name);
    //     let data = webhdf5.get_attribute_data(this.file_obj, obj_name, attr_name);
    //     if (metadata.typestr == "string") {
    //         return data;
    //     }
    //     else if (metadata.typestr == "int") {
    //         let accessor = (metadata.signed) ? "Int" : "Uint";
    //         accessor += ((metadata.size) * 8).toFixed() + "Array";
    //         return new globalThis[accessor](data.buffer);

    //     }
    //     else if (metadata.typestr == "float") {
    //         let accessor = "Float" + ((metadata.size) * 8).toFixed() + "Array";
    //         return new globalThis[accessor](data.buffer);
    //     }
    // }
}

class Group extends HasAttrs {
    constructor(file_obj, path) {
        super();
        this.path = path;
        this.file_obj = file_obj;
    }

    keys() {
        return webhdf5.get_names2(this.file_obj, this.path);
    }

    * values() {
        for (let name of this.keys()) {
            yield this.get(name);
        }
        return
    }

    * items() {
        for (let name of this.keys()) {
            yield [name, this.get(name)];
        }
        return
    }

    get_type(obj_path) {
        return webhdf5.get_type(this.file_obj, obj_path);
    }

    get(obj_name) {
        let fullpath = (/^\//.test(obj_name)) ? obj_name : this.path + "/" + obj_name;
        fullpath = normalizePath(fullpath);

        let type = this.get_type(fullpath);
        if (type == webhdf5.H5O_TYPE_GROUP) {
            return new Group(this.file_obj, fullpath);
        }
        else if (type == webhdf5.H5O_TYPE_DATASET) {
            return new Dataset(this.file_obj, fullpath);
        }
    }

    toString() {
        return "group";
    }
}

export class File extends Group {
    constructor(filename, mode = "r") {
        super(null, "/");
        let h5_mode = webhdf5[ACCESS_MODES[mode]];
        this.file_obj = new webhdf5.H5File(filename, h5_mode);
        this.mode = mode;
    }

    /**
     * Get the data in an attribute attached to an object
     *
     * @param {*} obj_name
     * @param {*} attr_name
     * @returns
     * @memberof File
     */
    get_attr(obj_name, attr_name) {
        //let attrs_metadata = webhdf5.get_attrs_metadata(this.file_obj, obj_name);
        //let metadata = attrs_metadata[attr_name];
        let metadata = webhdf5.get_attribute_metadata(this.file_obj, obj_name, attr_name);
        let data = webhdf5.get_attribute_data(this.file_obj, obj_name, attr_name);
        if (metadata.typestr == "string") {
            return data;
        }
        else if (metadata.typestr == "int") {
            let accessor = (metadata.signed) ? "Int" : "Uint";
            accessor += ((metadata.size) * 8).toFixed() + "Array";
            return new globalThis[accessor](data.buffer);

        }
        else if (metadata.typestr == "float") {
            let accessor = "Float" + ((metadata.size) * 8).toFixed() + "Array";
            return new globalThis[accessor](data.buffer);
        }
    }
}



class Dataset extends HasAttrs {
    constructor(file_obj, path) {
        super();
        this.path = path;
        this.file_obj = file_obj;
    }

    get metadata() {
        return webhdf5.get_dataset_metadata(this.file_obj, this.path);
    }

    get value() {
        let metadata = this.metadata;
        let data = webhdf5.get_dataset_data(this.file_obj, this.path);
        return process_data(data, metadata);
    }
}
