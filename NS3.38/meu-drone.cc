#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/netsimulyzer-module.h" // O módulo vital

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MeuDrone3D");

int main (int argc, char *argv[])
{
  // 1. Criar os nós (0 = Base, 1 = Drone)
  NodeContainer nodes;
  nodes.Create (2);

  // 2. Configurar Wi-Fi (Ad-hoc)
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211n);

  YansWifiPhyHelper wifiPhy;
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());

  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");

  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);

  // 3. Internet Stack
  InternetStackHelper internet;
  internet.Install (nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);

  // 4. MOBILIDADE
  MobilityHelper mobility;

  // --- Base (Nó 0): Fixa no chão ---
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes.Get (0));
  nodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0, 0, 0));

  // --- Drone (Nó 1): Waypoints ---
  mobility.SetMobilityModel ("ns3::WaypointMobilityModel");
  mobility.Install (nodes.Get (1));

  Ptr<WaypointMobilityModel> droneMobility = nodes.Get(1)->GetObject<WaypointMobilityModel>();
  
  // Define o trajeto (Subir, girar, descer)
  droneMobility->AddWaypoint (Waypoint (Seconds (0.0), Vector (0, 0, 0)));      // Chão
  droneMobility->AddWaypoint (Waypoint (Seconds (2.0), Vector (0, 0, 30)));     // Decola
  droneMobility->AddWaypoint (Waypoint (Seconds (5.0), Vector (50, 0, 30)));    // Frente
  droneMobility->AddWaypoint (Waypoint (Seconds (8.0), Vector (50, 50, 30)));   // Direita
  droneMobility->AddWaypoint (Waypoint (Seconds (12.0), Vector (0, 0, 30)));    // Volta base (alto)
  droneMobility->AddWaypoint (Waypoint (Seconds (15.0), Vector (0, 0, 0)));     // Pousa

  // 5. Aplicações (Tráfego UDP)
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1)); // Drone recebe
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (16.0));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (100));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.5)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0)); // Base envia
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (15.0));

  // =========================================================
  // 6. CONFIGURAÇÃO DO NETSIMULYZER (Baseado no seu exemplo)
  // =========================================================
  
  // Cria o "Orquestrador" que gerencia a visualização
  std::string outputFileName = "meu-drone.json";
  auto orchestrator = CreateObject<netsimulyzer::Orchestrator>(outputFileName);

  // Helper para configurar os nós no visualizador
  netsimulyzer::NodeConfigurationHelper nodeConfig(orchestrator);
  
  // Habilita o rastro de movimento (linha que segue o drone)
  nodeConfig.Set("EnableMotionTrail", BooleanValue(true));

  // --- Configura a Base (Nó 0) ---
  // Vamos usar um ícone de Smartphone ou genérico para a base
  nodeConfig.Set("Model", StringValue(netsimulyzer::models::SMARTPHONE)); 
  nodeConfig.Install(nodes.Get(0));

  // --- Configura o Drone (Nó 1) ---
  // Vamos usar o modelo 3D de Drone que vimos no exemplo
  nodeConfig.Set("Model", StringValue(netsimulyzer::models::LAND_DRONE));
  nodeConfig.Install(nodes.Get(1));

  // 7. Executar
  NS_LOG_UNCOND ("Simulacao rodando... output: " << outputFileName);
  Simulator::Stop (Seconds (16.0));
  Simulator::Run ();
  Simulator::Destroy ();
  
  return 0;
}
