export class PyModule
{
    private _type: 'PyModule';
    private constructor();
}

export class PyObject
{
    private _type: 'PyObject';
    private constructor();
}

export interface Interpreter
{
    import: (filename: string, allowReimport: boolean) => Promise<PyModule>;
    importSync: (filename: string, allowReimport: boolean) => PyModule;

    create: (module: PyModule, className: string, ...args: any[]) => Promise<PyObject>;
    createSync: (module: PyModule, className: string, ...args: any[]) => PyObject;

    call: (module: PyModule | PyObject, functionName: string, ...args: any[]) => Promise<unknown>;
    callSync: (module: PyModule | PyObject, functionName: string, ...args: any[]) => unknown;

    exec: (module: PyModule | PyObject, codeToRun: string) => Promise<unknown>;
    execSync: (module: PyModule | PyObject, codeToRun: string) => unknown;

    eval: (module: PyModule | PyObject, codeToRun: string) => Promise<unknown>;
    evalSync: (module: PyModule | PyObject, codeToRun: string) => unknown;

    fixlink: (fileName: string) => void;

    reimport: (directory: string) => void;

    addImportPath: (path: string) => void;

    developmentMode: (paths: string[]) => void;

    setSyncJsAndPyInCallback: (syncJsAndPy: boolean) => void;

    setPythonExecutable: (executable: string) => void;
}

export const interpreter: Interpreter;
