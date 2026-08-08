async function Main() {
  const interfaces = await Networking.Wifi.GetInterfaces();
  const iface = interfaces.find((i) => i.monitor);
  if (!iface) throw new Error("Error: No interface found!");

  const networks = await Networking.Wifi.Scan(iface.name);
  if (!networks.length) throw new Error("Error: No networks found!");
  for (let i = 0; i < networks.length; i++) {
    const network = networks[i];
    println(
      `${i + 1}. | ${network.ssid} | ${network.bssid} | Ch:${network.channel} | Signal:${network.signal}`,
    );
  }
  println("0. | Exit");

  const input = parseInt(await prompt("Select Network: "));
  if (input === 0) return;

  const target = networks[input - 1];
  if (!target) throw new Error("Error: Invalid network!");
  println("Deauthenticating Network...");
  await Networking.Wifi.Deauth(iface.name, target.bssid);
  const pcap = await Networking.Wifi.CaptureHandshake(iface.name, target.bssid);
  if (!pcap) throw new Error("Error: Couldn't capture handshake!");
  println(`Captured: ${pcap}`);

  const password = await Crypto.Hashcat.Decrypt(pcap);
  if (!password) throw new Error("Error: Couldn't decipher password!");
  await Networking.Wifi.Connect(target.ssid, password);

  await FileSystem.Remove(pcap);
}

Main();
