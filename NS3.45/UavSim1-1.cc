// scratch/UavSim.cc
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

#include <fstream>
#include <cmath>
#include <sstream>

using namespace ns3;
using namespace std;


NS_LOG_COMPONENT_DEFINE ("UavSim1-1");

int
main (int argc, char *argv[])
{
  // ---------------- CLI (apenas estes dois parâmetros) ----------------
  double distance = 30.0;   // metros (UAV em relação ao UE)
  double simTime = 10.0;    // segundos

  CommandLine cmd;
  cmd.AddValue ("distance", "Distância entre UE e UAV em metros", distance);
  cmd.AddValue ("simTime", "Tempo total de simulação em segundos", simTime);
  double uavSpeed = 1.0;   // velocidade do UAV (m/s)
  double targetX = 0.0, targetY = 0.0, targetZ = 0.0; // destino

  cmd.AddValue("uavSpeed", "Velocidade do UAV (m/s)", uavSpeed);
  cmd.AddValue("targetX", "Coordenada X de destino do UAV", targetX);
  cmd.AddValue("targetY", "Coordenada Y de destino do UAV", targetY);
  cmd.AddValue("targetZ", "Coordenada Z de destino do UAV", targetZ);

  cmd.Parse (argc, argv);

  // ---------------- parâmetros embutidos ----------------
  uint32_t packetSize = 512;         // bytes (tamanho do pacote)
  double interval = 1.0;             // segundos entre pacotes (1 pkt/s)
  double txPower = 16.0;             // dBm (potência Tx)
  double frequency = 2.4e9;          // Hz (frequência para Friis)

  // ---------------- criar nós ----------------
  NodeContainer nodes;
  nodes.Create (2); // 0 = UE (fixo), 1 = UAV (afastado)

  // ---------------- configurar WiFi ----------------
  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211g);

  YansWifiPhyHelper phy;
  phy.Set ("TxPowerStart", DoubleValue (txPower));
  phy.Set ("TxPowerEnd", DoubleValue (txPower));
  phy.Set ("RxGain", DoubleValue (0));
  phy.Set ("TxGain", DoubleValue (0));

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  // Modelo de perda log-distância: referência em 30m
  channel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel",
                              "Exponent", DoubleValue (3.0),           // atenua mais rápido que o espaço livre
                              "ReferenceDistance", DoubleValue (30.0), // até 30m quase sem perda
                              "ReferenceLoss", DoubleValue (0.0)); // perda em dB em 30m

  // Adiciona desvanecimento multipercurso (mais realista)
  channel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");

  phy.SetChannel (channel.Create ());

  WifiMacHelper mac;
  mac.SetType ("ns3::AdhocWifiMac");

  NetDeviceContainer devices = wifi.Install (phy, mac, nodes);

  // ---------------- mobilidade ----------------
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));         // UE fixo
  positionAlloc->Add (Vector (distance, 0.0, 0.0));    // UAV afastado
  mobility.SetPositionAllocator (positionAlloc);

  // Mobilidade: UE fixo, UAV móvel em direção ao ponto destino
  mobility.Install (nodes.Get(0)); // UE fixo

  // UAV com movimento constante em direção à coordenada final
  MobilityHelper uavMobility;
  Ptr<ListPositionAllocator> uavPos = CreateObject<ListPositionAllocator> ();
  uavPos->Add(Vector(distance, 0.0, 0.0)); // posição inicial
  uavMobility.SetPositionAllocator(uavPos);
  uavMobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
  uavMobility.Install(nodes.Get(1));

  // Definir velocidade do UAV em direção ao ponto de destino
  Ptr<ConstantVelocityMobilityModel> mob = nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>();
  Vector initialPos = mob->GetPosition();
  Vector direction = Vector(targetX - initialPos.x, targetY - initialPos.y, targetZ - initialPos.z);
  double length = sqrt(direction.x*direction.x + direction.y*direction.y + direction.z*direction.z);
  if (length > 0)
    mob->SetVelocity(Vector(uavSpeed * direction.x / length,
                            uavSpeed * direction.y / length,
                            uavSpeed * direction.z / length));
  else
    mob->SetVelocity(Vector(0.0, 0.0, 0.0));


  // ---------------- pilha IP ----------------
  InternetStackHelper stack;
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // ---------------- aplicações UDP ----------------
  uint16_t port = 9000;
  UdpServerHelper server (port);
  ApplicationContainer serverApps = server.Install (nodes.Get (1));
  serverApps.Start (Seconds (0.5));
  serverApps.Stop (Seconds (simTime + 0.1));

  UdpClientHelper client (interfaces.GetAddress (1), port);
  uint32_t nPackets = (uint32_t) max(1.0, floor((simTime - 1.0) / interval + 0.5));
  client.SetAttribute ("MaxPackets", UintegerValue (nPackets));
  client.SetAttribute ("Interval", TimeValue (Seconds (interval)));
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  ApplicationContainer clientApps = client.Install (nodes.Get (0));
  clientApps.Start (Seconds (1.0));
  clientApps.Stop (Seconds (simTime + 0.05));

  // ---------------- Flow monitor ----------------
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Stop (Seconds (simTime + 0.5));
  Simulator::Run ();

  // ---------------- coletar estatísticas ----------------
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  Ptr<MobilityModel> m0 = nodes.Get (0)->GetObject<MobilityModel> ();
  Ptr<MobilityModel> m1 = nodes.Get (1)->GetObject<MobilityModel> ();
  Vector p0 = m0->GetPosition ();
  Vector p1 = m1->GetPosition ();
  double realDistance = sqrt ((p0.x - p1.x)*(p0.x - p1.x) +
                                   (p0.y - p1.y)*(p0.y - p1.y) +
                                   (p0.z - p1.z)*(p0.z - p1.z));

  double app_bps = (double) packetSize * 8.0 / interval; // bps
  double app_kbps = app_bps / 1000.0;

  ofstream logFile ("scratch/UavSim1-1-D" + to_string((int)distance) + "-T" + to_string((int)simTime) + ".log", ios::out);

  // função lambda para imprimir no terminal e no log
  auto log = [&](const string &msg) {
    cout << msg;
    logFile << msg;
    logFile.flush();
  };

  // ---------------- saída de configuração ----------------
  log("=== Configuração da Simulação ===\n");
  log("Tempo de simulação (s): " + to_string(simTime) + "\n");
  log("Distância solicitada (m): " + to_string(distance) + "\n");
  log("Distância real UE <-> UAV (m): " + to_string(realDistance) + "\n");
  log("Tamanho do pacote (bytes): " + to_string(packetSize) + "\n");
  log("Intervalo entre pacotes (s): " + to_string(interval) + " (1 pkt/s)\n");
  log("Vazão configurada (app-level): " + to_string(app_kbps) + " kbps\n");
  log("Velocidade UAV (m/s): " + to_string(uavSpeed) + "\n");
  log("Destino UAV (x,y,z): (" + to_string(targetX) + "," + to_string(targetY) + "," + to_string(targetZ) + ")\n");
  log("Potência Tx (dBm): " + to_string(txPower) + "\n");
  log("Frequência (GHz): " + to_string(frequency/1e9) + "\n\n");

  // ---------------- resultados ----------------
  log("=== Resultados da Simulação ===\n");
  for (auto &flow : stats)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (flow.first);
      double duration = simTime;
      double throughput_kbps = (flow.second.rxBytes * 8.0) / duration / 1000.0;

      ostringstream addrStream;
      addrStream << t.sourceAddress << " -> " << t.destinationAddress;
      log("  Fluxo " + to_string(flow.first) + " (" + addrStream.str() + ")\n");

      log("  Pacotes enviados: " + to_string(flow.second.txPackets) + "\n");
      log("  Pacotes recebidos: " + to_string(flow.second.rxPackets) + "\n");
      log("  Perdidos: " + to_string(flow.second.txPackets - flow.second.rxPackets) + "\n");
      log("  Bytes recebidos: " + to_string(flow.second.rxBytes) + "\n");
      log("  Vazão observada: " + to_string(throughput_kbps) + " kbps\n");
      if (flow.second.txPackets > 0)
        {
          double delivery = flow.second.rxPackets * 100.0 / flow.second.txPackets;
          log("  Taxa de entrega: " + to_string(delivery) + " %\n");
        }
      log("\n");
    }

  logFile.close ();
  Simulator::Destroy ();
  return 0;
}

