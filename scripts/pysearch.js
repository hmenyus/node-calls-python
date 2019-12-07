const path = require("path");
const fs = require('fs')

let pathenv = process.env.PATH.split(";");
let arg = process.argv[2];

for (let p of pathenv)
{
    if (p.toLowerCase().includes("python"))
    {
        if (fs.existsSync(path.join(p, "python.exe")) || fs.existsSync(path.join(p, "python")))
        {
            if (arg == "lib")
            {
                fs.readdir(path.join(p, "libs"), function(err, items) {
                    let length = 0;
                    let result = "";
                    for (let i of items)
                    {
                        if (i.startsWith("python"))
                        {
                            if (i.endsWith(".lib") || i.endsWith(".a"))
                            {
                                if (length < i.length)
                                {
                                    length = i.length;
                                    result = i;
                                }
                            }
                        }
                    }

                    let index = result.lastIndexOf(".");
                    if (index != -1)
                        result = result.substring(0, index);

                    console.log("-l" + result);
                });
            }
            else
                console.log(path.join(p, arg));

            return;
        }
    }
}

console.log("Cannot find PYTHON");
