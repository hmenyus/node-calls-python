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

let linkerLine = stdout.toString().trim();

// conda hack starts here
try
{
    const condaBase = execSync("conda info --base 2>&1");
    if (condaBase)
    {
        const condaBaseString = condaBase.toString().trim();
        if (linkerLine.includes(condaBaseString))
            linkerLine += " -L" + path.join(condaBaseString, "lib")
    }
}
catch(e)
{
}

console.log(linkerLine);
