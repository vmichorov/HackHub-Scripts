const signalColors = [
  "🔴", // 0
  "🟠", // 1
  "🟡", // 2
  "🟢", // 3
];

async function Main() {
  Shell.lock();
  const interfaces = await Networking.Wifi.GetInterfaces();
  const iface = interfaces.find((i) => i.monitor);
  if (!iface) {
    println({ text: "Error: No interface found!", color: "red" });
    Shell.unlock();
    return;
  }

  const networks = await Networking.Wifi.Scan(iface.name);
  if (!networks.length) {
    println({ text: "Error: No networks found!", color: "red" });
    Shell.unlock();
    return;
  }

  printTable(
    networks.map((n, i) => ({
      "": i + 1,
      SSID: n.ssid,
      BSSID: n.bssid,
      CHANNEL: n.channel,
      SIGNAL: signalColors[n.signal],
    })),
  );

  Shell.unlock();
  const input = parseInt(await prompt("Select Network: "));
  Shell.lock();

  const target = networks[input - 1];
  if (!target) {
    println({ text: "Error: Invalid network!", color: "red" });
    Shell.unlock();
    return;
  }

  let isPassSaved = false;
  let password = "";
  const files = await FileSystem.ReadDir("scripts");
  const passFile = files!.find((f) => f.name === "passwords");
  let initialContent = "";

  if (!passFile) {
    println("Creating passwords file...");
    await FileSystem.WriteFile("./scripts/passwords.txt", "");
    isPassSaved = false;
  } else {
    println("Searching for password in file...");
    const content = await FileSystem.ReadFile("./scripts/passwords.txt");
    initialContent = content;
    const lines = content.split(/\r?\n/);
    if (lines.length) {
      for (let line of lines) {
        const [name, pass] = line.split(":");
        if (name === target.ssid) {
          isPassSaved = true;
          println("Retrieving password from file...");
          password = pass;
          break;
        }
      }
    }
  }
  if (!isPassSaved) {
    println("Password not saved, deauthenticating network...");
    await Networking.Wifi.Deauth(iface.name, target.bssid);
    const pcap = await Networking.Wifi.CaptureHandshake(
      iface.name,
      target.bssid,
    );
    if (!pcap) {
      println({ text: "Error: Couldn't capture handshake!", color: "red" });
      Shell.unlock();
      return;
    }

    println("Deciphering password...");
    const decrypt = await Crypto.Hashcat.Decrypt(pcap);
    if (!decrypt) {
      println({ text: "Error: Couldn't decipher password!", color: "red" });
      Shell.unlock();
      return;
    }
    await FileSystem.Remove(pcap);
    password = decrypt;

    println("Saving password to file...");
    const newPassFileContent = `${initialContent}\n${target.ssid}:${password}`;
    await FileSystem.WriteFile(
      "./scripts/passwords.txt",
      newPassFileContent.trim(),
    );
  }

  println("Connecting to network...");
  await Networking.Wifi.Connect(target.ssid, password);

  Shell.unlock();
}

Main();
