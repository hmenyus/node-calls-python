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

if (stdout)
{
    const splits = stdout.toString().trim().split(" ");

    // conda hack starts here
    try
    {
        const condaBase = execSync("conda info --base 2>&1");
        if (condaBase)
            splits.push("-L" + path.join(condaBase.toString().trim(), "lib"));
    }
    catch(e)
    {
    }

    const result = [];
    splits.forEach(s => {
        if (s.startsWith("-L"))
            result.push("-Wl,-rpath," + s.substring(2));
    });

    console.log(" " + result.join(" "));
}
