const { execSync } = require("child_process");

const stdout = execSync("python3-config --ldflags --embed");
if (stdout)
{
    const splits = stdout.toString().trim().split(" ");

    const result = [];
    splits.forEach(s => {
        if (s.startsWith("-L"))
            result.push(s.substring(2));
    });

    result.forEach((r, i) => {
        if (i == 0)
        {
            if (result.length > 1)
                console.log("-Wl,-rpath," + r +" \\");
            else
                console.log("-Wl,-rpath," + r);
        }
        else if (i + 1 == result.length)
            console.log("\t-Wl,-rpath," + r);
        else
            console.log("\t-Wl,-rpath," + r + " \\");
    });
}
