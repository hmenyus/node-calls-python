import { interpreter as py } from 'node-calls-python';
import py2 from 'node-calls-python';

import {join, dirname } from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

let pyfile = join(__dirname, "nodetest.py");

let pymodule = py.importSync(pyfile);

console.log(py.callSync(pymodule, "hello"));
console.log(py.callSync(pymodule, "concatenate", "aaa", "bbb"));
