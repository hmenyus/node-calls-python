export class PyModule
{
    private _type: 'PyModule';
    private constructor() {}
}

export class PyObject
{
    private _type: 'PyObject';
    private constructor() {}
}

export interface Interpreter
{
    import: (filename: string) => Promise<PyModule>;
    importSync: (filename: string) => PyModule;

    create: (module: PyModule, className: string, ...args: any[]) => Promise<PyObject>;
    createSync: (module: PyModule, className: string, ...args: any[]) => PyObject;

    call: (module: PyModule | PyObject, functionName: string, ...args: any[]) => Promise<unknown>;
    callSync: (module: PyModule | PyObject, functionName: string, ...args: any[]) => unknown;

    fixlink: (fileName: string) => void;

    addImportPath: (path: string) => void;
}

export const interpreter: Interpreter;