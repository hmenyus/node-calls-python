const { execSync } = require("child_process");

const stdout = execSync("python3-config --ldflags --embed");
if (stdout)
{
    const splits = stdout.toString().trim().split(" ");

    const result = [];
    splits.forEach(s => {
        if (s.startsWith("-L"))
            result.push("-Wl,-rpath," + s.substring(2));
    });

    console.log(result.join(" "));
}
