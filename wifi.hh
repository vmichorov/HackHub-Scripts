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
  println("Deauthenticating Network...");
  await Networking.Wifi.Deauth(iface.name, target.bssid);
  const pcap = await Networking.Wifi.CaptureHandshake(iface.name, target.bssid);
  if (!pcap) {
    println({ text: "Error: Couldn't capture handshake!", color: "red" });
    Shell.unlock();
    return;
  }

  println("Deciphering password...");
  const password = await Crypto.Hashcat.Decrypt(pcap);
  if (!password) {
    println({ text: "Error: Couldn't decipher password!", color: "red" });
    Shell.unlock();
    return;
  }

  println("Connecting to network...");
  await Networking.Wifi.Connect(target.ssid, password);

  await FileSystem.Remove(pcap);
  Shell.unlock();
}

Main();
