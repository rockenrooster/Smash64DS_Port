// Ghidra headless post-script: decompile a list of target addresses to C drafts.
//
// Reads a targets file (one "0xADDR name size" per line), decompiles each
// function (creating one at that address if Ghidra didn't auto-detect it), and
// writes the C pseudocode to <outDir>/<0xADDR>.c. These drafts are NOT matching
// C -- they're scaffolds we hand to the LLM tier to fix up to a byte-match.
//
// Invoked by analyzeHeadless via:
//   -postScript DecompDump.java <targetsPath> <outDir>
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.PrintWriter;

public class DecompDump extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) {
            println("usage: DecompDump <targetsPath> <outDir>");
            return;
        }
        String targetsPath = args[0];
        File outDir = new File(args[1]);
        outDir.mkdirs();

        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);

        int ok = 0, fail = 0;
        BufferedReader br = new BufferedReader(new FileReader(targetsPath));
        String line;
        while ((line = br.readLine()) != null) {
            line = line.trim();
            if (line.isEmpty()) continue;
            String tok = line.split("\\s+")[0];      // 0xADDR
            Address addr;
            try {
                addr = toAddr(Long.decode(tok));
            } catch (Exception e) {
                fail++; continue;
            }
            Function f = getFunctionAt(addr);
            if (f == null) {
                f = createFunction(addr, null);      // force a function if missed
            }
            if (f == null) { fail++; continue; }

            DecompileResults res = di.decompileFunction(f, 60, monitor);
            if (res != null && res.decompileCompleted()
                    && res.getDecompiledFunction() != null) {
                PrintWriter pw = new PrintWriter(new File(outDir, tok + ".c"));
                pw.print(res.getDecompiledFunction().getC());
                pw.close();
                ok++;
            } else {
                fail++;
            }
        }
        br.close();
        println("DecompDump done: ok=" + ok + " fail=" + fail);
    }
}
