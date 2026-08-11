async function Main() {
  const host = await prompt("Enter IP Address: ");
  Shell.lock();
  if (!Networking.IsIp(host)) {
    println({ text: "Error: IP is invalid!", color: "red" });
    Shell.unlock();
    return;
  }

  println("Fetching subnet information...");
  const subnet = await Networking.GetSubnet(host);
  if (!subnet) {
    println({ text: "Error: Couldn't find subnet!", color: "red" });
    Shell.unlock();
    return;
  }

  println("Fetching ports...");
  const ports = await subnet.GetPorts();
  if (!ports.length) {
    println({ text: "Info: No ports found!", color: "yellow" });
    Shell.unlock();
    return;
  }

  println("Scanning ports...");
  const portsData: Networking.Port[] = [];
  const portsStatuses: Record<number, boolean> = {};
  for (let p of ports) {
    const pData = await subnet.GetPortData(p);
    const isOpen = await subnet.PingPort(p);
    portsStatuses[p] = isOpen;
    if (pData) {
      portsData.push(pData);
    }
  }
  printTable(
    portsData
      .sort((a, b) => a.internal - b.internal)
      .map((p) => ({
        PORT: p.external,
        STATUS: portsStatuses[p.internal] ? "OPEN" : "CLOSED",
        TARGET: p.target!,
        SERVICE: p.service ? p.service : "",
        VERSION: p.version ? p.version : "",
      })),
  );

  Shell.unlock();
  const port = parseInt(await prompt("Select Port To Exploit: "));
  Shell.lock();

  const data = portsData.find((p) => p.external === port);
  if (!data) {
    println({ text: "Error: Couldn't get port data!", color: "red" });
    Shell.unlock();
    return;
  }

  const service = data.service;
  if (!service) {
    println({ text: "Error: Service not found!", color: "red" });
    Shell.unlock();
    return;
  }
  const serviceVersion = data.version;
  if (!serviceVersion) {
    println({ text: "Error: Version not found!", color: "red" });
    Shell.unlock();
    return;
  }
  const version = serviceVersion.split(" ")[1];

  const ms = GetMetasploit();

  println("Searching for exploits...");
  let target;
  const modules = await ms.Search(service);

  printTable(
    modules.map((m, i) => ({
      "": i,
      NAME: m.name,
      DESCRIPTION: m.description,
      RANK: m.rank,
      "DISCLOSURE DATE": m.disclosureDate,
    })),
  );

  Shell.unlock();
  const input = parseInt(await prompt("Select Exploit: "));
  Shell.lock();
  target = modules[input];

  println(`Using exploit ${target.name}`);

  println("Setting Options...");
  await ms.Use(target.name);
  await ms.SetOption("RHOST", host);
  await ms.SetOption("RPORT", port);
  await ms.SetOption("Version", version);

  println("Exploiting Service...");
  await ms.Exploit();
  Shell.unlock();
}

Main();
