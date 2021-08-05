import ModuleFactory from './h5js_module.js';

export var Module = null;
export const ready = ModuleFactory({noInitialRun: true}).then((result) => { Module = result });

export const UPLOADED_FILES = [];

export function upload_file() {
    let file = this.files[0]; // only one file allowed
    let datafilename = file.name;
    let reader = new FileReader();
    reader.onloadend = function (evt) {
        let data = evt.target.result;
        Module.FS.writeFile(datafilename, new Uint8Array(data));
        if (!UPLOADED_FILES.includes(datafilename)) {
            UPLOADED_FILES.push(datafilename);
            console.log("file loaded:", datafilename);
        }
        else {
            console.log("file updated: ", datafilename)
        }
    }
    reader.readAsArrayBuffer(file);
    this.value = "";
}


export const ACCESS_MODES = {
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
    let metadata = Module.get_attribute_metadata(file_obj, obj_name, attr_name);
    let data = Module.get_attribute_data(file_obj, obj_name, attr_name);
    return process_data(data, metadata)
}

function process_data(data, metadata) {
    if (metadata.typestr == "string") {
        return data;
    }
    else if (metadata.typestr == "int") {
        let accessor = (metadata.size > 4) ? "Big" : "";
        accessor += (metadata.signed) ? "Int" : "Uint";
        accessor += ((metadata.size) * 8).toFixed() + "Array";
        return new globalThis[accessor](data.buffer);

    }
    else if (metadata.typestr == "float") {
        let accessor = "Float" + ((metadata.size) * 8).toFixed() + "Array";
        return new globalThis[accessor](data.buffer);
    }
}


const int_fmts = new Map([[1, 'b'], [2, 'h'], [4, 'i'], [8, 'q']]);
const float_fmts = new Map([[2, 'e'], [4, 'f'], [8, 'd']]);

function metadata_to_dtype(metadata) {
    if (metadata.typestr == "string") {
        let length_str = metadata.vlen ? "" : String(metadata.size);
        return `${length_str}S`;
    }
    else if (metadata.typestr == "int") {
        let fmt = int_fmts.get(metadata.size);
        if (!metadata.signed) {
            fmt = fmt.toUpperCase();
        }
        return ((metadata.littleEndian) ? "<" : ">") + fmt;
    }
    else if (metadata.typestr == "float") {
        let fmt = float_fmts.get(metadata.size);
        return ((metadata.littleEndian) ? "<" : ">") + fmt;
    }
    else {
        return "unknown";
    }
}   

class HasAttrs {
    get attrs() {
        let attrs_metadata = Module.get_attrs_metadata(this.file_obj, this.path);
        let attrs = {};
        for (let [name, metadata] of Object.entries(attrs_metadata)) {
            Object.defineProperty(attrs, name, {
                get: () => ({
                    value: get_attr(this.file_obj, this.path, name),
                    metadata,
                    dtype: metadata_to_dtype(metadata)
                }),
                enumerable: true
            });
        }
        return attrs;
        
    }

}

export class Group extends HasAttrs {
    constructor(file_obj, path) {
        super();
        this.path = path;
        this.file_obj = file_obj;
    }

    keys() {
        return Module.get_names2(this.file_obj, this.path);
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
        return Module.get_type(this.file_obj, obj_path);
    }

    get(obj_name) {
        let fullpath = (/^\//.test(obj_name)) ? obj_name : this.path + "/" + obj_name;
        fullpath = normalizePath(fullpath);

        let type = this.get_type(fullpath);
        if (type == Module.H5O_TYPE_GROUP) {
            return new Group(this.file_obj, fullpath);
        }
        else if (type == Module.H5O_TYPE_DATASET) {
            return new Dataset(this.file_obj, fullpath);
        }
    }

    toString() {
        return `Group(file_obj=${this.file_obj.getId()}, path=${this.path})`;
    }
}

export class File extends Group {
    constructor(filename, mode = "r") {
        super(null, "/");
        let h5_mode = Module[ACCESS_MODES[mode]];
        this.file_obj = new Module.H5File(filename, h5_mode);
        this.filename = filename;
        this.mode = mode;
    }
}



export class Dataset extends HasAttrs {
    constructor(file_obj, path) {
        super();
        this.path = path;
        this.file_obj = file_obj;
    }

    get metadata() {
        return Module.get_dataset_metadata(this.file_obj, this.path);
    }

    get dtype() {
        return metadata_to_dtype(this.metadata);
    }

    get shape() {
        return this.metadata.shape;
    }

    get value() {
        let metadata = this.metadata;
        let data = Module.get_dataset_data(this.file_obj, this.path, null, null);
        return process_data(data, metadata);
    }

    slice(ranges) {
        // interpret ranges as [start, stop], with one per dim.
        let metadata = this.metadata;
        let shape = metadata.shape;
        let ndims = shape.length;
        let count = shape.map((s, i) => BigInt(Math.min(s, ranges?.[i]?.[1] ?? s) - Math.max(0, ranges?.[i]?.[0] ?? 0)));
        let offset = shape.map((s, i) => BigInt(Math.min(s, Math.max(0, ranges?.[i]?.[0] ?? 0))));
        //let count_array = new BigUint64Array(count);
        //let offset_array = new BigUint64Array(offset);
        //let count_ptr = Module._malloc(count_array.byteLength);
        //let offset_ptr = Module._malloc(offset_array.byteLength);
        //Module.HEAPU8.set(new Uint8Array(count_array.buffer), count_ptr);
        //Module.HEAPU8.set(new Uint8Array(offset_array.buffer), offset_ptr);
        //console.log(count, count_array, count_ptr);
        //console.log(offset, offset_array, offset_ptr);
        //let data = Module.get_dataset_data(this.file_obj, this.path, count_ptr, offset_ptr);
        console.log(count, offset);
        let data = Module.get_dataset_data(this.file_obj, this.path, count, offset);
        //Module._free(count_ptr);
        //Module._free(offset_ptr);
        return process_data(data, metadata);
    }
}
