#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/netsimulyzer-module.h" // Importante para a visualização
#include "ns3/netsimulyzer-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DroneSimExample");

int main (int argc, char *argv[])
{
  // 1. Criar os nós (0 = Ground Station, 1 = UAV)
  NodeContainer nodes;
  nodes.Create (2);

  // 2. Configurar a conexão Wi-Fi (Ad-hoc) entre eles
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211n);

  YansWifiPhyHelper wifiPhy;
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());

  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");

  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);

  // 3. Instalar a pilha de Internet (IP)
  InternetStackHelper internet;
  internet.Install (nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);

  // 4. Configurar a Mobilidade (A parte visual 3D!)
  MobilityHelper mobility;

  // --- Nó 0: Ground Station (Fixo no chão) ---
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes.Get (0));
  // Posição: X=0, Y=0, Z=0
  nodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0, 0, 0));

  // --- Nó 1: UAV (Movimento via Waypoints) ---
  // Vamos fazer o drone voar em um quadrado e subir altitude
  mobility.SetMobilityModel ("ns3::WaypointMobilityModel");
  mobility.Install (nodes.Get (1));

  Ptr<WaypointMobilityModel> uavMobility = nodes.Get(1)->GetObject<WaypointMobilityModel>();
  
  // Define os pontos de passagem (Tempo, Posição X, Y, Z)
  uavMobility->AddWaypoint (Waypoint (Seconds (0.0), Vector (0, 0, 0)));      // Começa no chão junto da base
  uavMobility->AddWaypoint (Waypoint (Seconds (1.0), Vector (0, 0, 10)));     // Sobe 10m (Decolagem)
  uavMobility->AddWaypoint (Waypoint (Seconds (5.0), Vector (50, 0, 10)));    // Voa para frente
  uavMobility->AddWaypoint (Waypoint (Seconds (10.0), Vector (50, 50, 20)));  // Voa para o lado e sobe para 20m
  uavMobility->AddWaypoint (Waypoint (Seconds (15.0), Vector (0, 50, 20)));   // Volta alinhado
  uavMobility->AddWaypoint (Waypoint (Seconds (20.0), Vector (0, 0, 0)));     // Pousa na base

  // 5. Aplicações (Tráfego de Rede)
  // Ground Station manda pacotes para o UAV
  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1)); // UAV é o servidor (recebe)
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (20.0));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (100));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.2))); // 1 pacote a cada 0.2s
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0)); // Base é o cliente (envia)
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (19.0));

  // 6. Configuração do NETSIMULYZER (Gera o JSON)
  NetSimulyzerHelper netsimulyzer;
  // Nome do arquivo de saída
  netsimulyzer.SetDeviceAttribute ("OutputFileName", StringValue ("simulacao-drone.json"));
  
  // Opcional: Definir qual nó é o que (para aparecer na legenda)
  // Mas o visualizador detecta os nós automaticamente.
  
  netsimulyzer.Enable (nodes);

  // 7. Rodar Simulação
  NS_LOG_UNCOND ("Rodando simulação de Drone...");
  Simulator::Stop (Seconds (21.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_UNCOND ("Pronto! Arquivo 'simulacao-drone.json' gerado.");

  return 0;
}