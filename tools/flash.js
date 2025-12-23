// DSS script for flashing MSPM0G3507
// Called by flash.sh

// Connect to target
var ds = Packages.com.ti.ccstudio.scripting.environment.ScriptingEnvironment.instance();
var debugServer = ds.getServer("DebugServer.1");

debugServer.setConfig("targetConfigs/MSPM0G3507.ccxml");

var debugSession = debugServer.openSession(".*", "MSPM0G3507");

// Connect
debugSession.target.connect();

// Load program
debugSession.memory.loadProgram("Debug/Motion_Music_Studio.out");

// Run
debugSession.target.run();

// Disconnect
debugSession.target.disconnect();
debugServer.stop();

print("Flash complete!");