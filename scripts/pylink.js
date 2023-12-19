const { execSync } = require("child_process");
const path = require('path');

let stdout;
try
{
    stdout = execSync("python3-config --ldflags --embed");
}
catch(e)
{
    stdout = execSync("python3-config --ldflags");
}

let linkerLine = stdout.toString();

// conda hack starts here
try
{
    const condaBase = execSync("conda info --base");
    if (linkerLine.includes(condaBase))
        linkerLine += " -L" + condaBase + path.join(condaBase, "lib")
}
catch(e)
{
}

console.log(linkerLine);
