/* -*- 0Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/gnuplot.h"
#include "Observador.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("fuentes12");

Observador simulacion (double tasaMediaIn, double tasaMediaOut, uint32_t tamPaqueteIn,
  uint32_t tamPaqueteOut, uint32_t usuariosXbus, uint32_t numSwitches,
  uint32_t numServidores, double retardo);

int aleatorio(uint32_t numServidores);

int
main (int argc, char *argv[])
{
  GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));
  Time::SetResolution (Time::NS);

  // Tasas en KB/s
  double tasaMediaIn = 5.36;
  double tasaMediaOut = 7.13;
  // Paquetes en Bytes
  uint32_t tamPaqueteIn = 136;
  uint32_t tamPaqueteOut = 71;
  uint32_t usuariosXbus = 15;
  uint32_t numSwitches = 15;
  uint32_t numServidores = 10;
  double retardo = 2;

  // Parametros por linea de comandos
  CommandLine cmd;
  cmd.AddValue ("tasaMediaIn", "Tasa media de entrada en los equipos", tasaMediaIn);
  cmd.AddValue ("tasaMediaOut", "Tasa media de salida en los equipos", tasaMediaOut);
  cmd.AddValue ("tamPaqueteIn", "Tamaño de paquetes de entrada", tamPaqueteIn);
  cmd.AddValue ("tamPaqueteOut", "Tamaño de paquetes de salida", tamPaqueteOut);
  cmd.AddValue ("usuariosXbus", "Equipos de jugadores por cada bus CSMA", usuariosXbus);
  cmd.AddValue ("numSwitches", "Numero de switches", numSwitches);
  cmd.AddValue ("numServidores", "Numero de servidores", numServidores);
  cmd.AddValue ("retardo", "Retardo de envio", retardo);
  cmd.Parse (argc,argv);

  return 0;
}

Observador simulacion (double tasaMediaIn, double tasaMediaOut, uint32_t tamPaqueteIn,
  uint32_t tamPaqueteOut, uint32_t usuariosXbus, uint32_t numSwitches,
  uint32_t numServidores, double retardo)
{
  // Variables aleatorias del sistema
  Ptr<NormalRandomVariable> tasaIn = CreateObject<NormalRandomVariable> ();
  Ptr<NormalRandomVariable> tasaOut = CreateObject<NormalRandomVariable> ();
  tasaIn->SetAttribute ("Mean", DoubleValue(tasaMediaIn));
  tasaOut->SetAttribute ("Mean", DoubleValue(tasaMediaOut));

  // Variable para calcular los estadisticos
  Observador observador;

  // Creacion de servidores con conexion P2P a un switch
  NodeContainer p2pNodes;
  p2pNodes.Create (numServidores+1);

  // Creacion de bus CSMA de switches
  NodeContainer csmaSwitchesNodes;
  csmaSwitchesNodes.Add (p2pNodes.Get (0));
  csmaSwitchesNodes.Create (numSwitches);

  // Creacion de bus CSMA en cada switch con n equipos
  NodeContainer csmaUsuariosNodes [numSwitches];
  for (uint32_t i=0; i<numSwitches; i++)
  {
    csmaUsuariosNodes[i].Add (p2pNodes.Get (i));
    csmaUsuariosNodes[i].Create (usuariosXbus);
  }

  // Instalacion de los servidores a los P2P con el switch
  // El nodo 0 es el switch y el resto los servidores conectados a el
  PointToPointHelper pointToPoint [numServidores];
  NetDeviceContainer p2pDevices [numServidores];
  for(uint32_t i=0; i<numServidores; i++)
  {
    pointToPoint[i].SetDeviceAttribute ("DataRate", StringValue ("54Mbps"));
    pointToPoint[i].SetChannelAttribute ("Delay", StringValue ("1ms"));
    p2pDevices[i] = pointToPoint[i].Install (p2pNodes.Get (0), p2pNodes.Get (i+1));
  }

  // Instalacion del bus CSMA de switches
  CsmaHelper csmaSwitches;
  NetDeviceContainer csmaSwitchesDevices;
  csmaSwitches.SetChannelAttribute ("DataRate", StringValue ("54Mbps"));
  csmaSwitches.SetChannelAttribute ("Delay", StringValue ("2ms"));
  csmaSwitchesDevices = csmaSwitches.Install (csmaSwitchesNodes);

  // Instalacion de los buses CSMA de los usuarios conectados a cada switch
  CsmaHelper csmaUsuarios [numSwitches];
  NetDeviceContainer csmaUsuariosDevices [numSwitches];
  for(uint32_t i=0; i<numSwitches; i++)
  {
    csmaUsuarios[i].SetChannelAttribute ("DataRate", StringValue ("54Mbps"));
    csmaUsuarios[i].SetChannelAttribute ("Delay", StringValue ("2ms"));
    csmaUsuariosDevices[i] = csmaUsuarios[i].Install (csmaUsuariosNodes[i]);
  }

  // Instalamos la pila TCP/IP en todos los nodos
  InternetStackHelper stack;
  for(uint32_t i=1; i<=numServidores; i++)
  {
    stack.Install (p2pNodes.Get (i));
  }
  stack.Install (csmaSwitchesNodes);
  for(uint32_t i=1; i<=usuariosXbus; i++)
  {
    stack.Install (csmaUsuariosNodes.Get (i));
  }

  // Asignamos direcciones a cada una de las interfaces
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer p2pInterfaces [numServidores];
  for(uint32_t i=0; i<numServidores; i++)
  {
    std::ostringstream network;
    network << "10.1." << (i+1) << ".0";
    address.SetBase (network.str ().c_str (), "255.255.255.0");
    p2pInterfaces[i] = address.Assign (p2pDevices[i]);
  }

  Ipv4InterfaceContainer csmaSwitchesInterfaces;
  address.SetBase ("10.1.2.0", "255.255.255.0");
  csmaSwitchesInterfaces = address.Assign (csmaSwitchesDevices);

  Ipv4InterfaceContainer csmaUsuariosInterfaces [numSwitches];
  for(uint32_t i=0; i<numSwitches; i++)
  {
    std::ostringstream network;
    network << "10.1." << ((i+1) + (numServidores+1)) << ".0";
    address.SetBase (network.str ().c_str (), "255.255.255.0");
    csmaUsuariosInterfaces[i] = address.Assign (csmaUsuariosDevices[i]);
  }

  // Calculamos las rutas del escenario. Con este comando, los
  //     nodos de la red de área local definen que para acceder
  //     al nodo del otro extremo del enlace punto a punto deben
  //     utilizar el primer nodo como ruta por defecto.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Creamos las aplicaciones serveridoras UDP en los nodos servidores
  uint16_t port;
  for(uint32_t i=1; i<=numServidores; i++)
  {
    port = 5000;
    UdpServerHelper server (port);
    ApplicationContainer apps = server.Install (p2pNodes.Get (i));
    apps.Start (Seconds (1.0));
    apps.Stop (Seconds (10.0));
  }

  // Creamos las aplicaciones clientes UDP en los nodos clientes eligiendo el servidor de forma aleatoria.
  Time interPacketInterval = Seconds (0.025);
  uint32_t maxPacketCount = 10000;
  for(uint32_t i=0; i<numSwitches; i++)
  {
    for(uint32_t j=0; j<usuariosXbus; j++)
    {
      int servidor = aleatorio(numServidores);
      Address serverAddress = Address (p2pInterfaces[servidor].GetAddress (1));
      UdpClientHelper client (serverAddress, port);
      client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      client.SetAttribute ("Interval", TimeValue (interPacketInterval));
      client.SetAttribute ("PacketSize", UintegerValue (tamPaqueteOut));
      ApplicationContainer apps = client.Install (csmaUsuariosNodes[i].Get (j));

      apps.Start (Seconds (2.0));
      apps.Stop (Seconds (10.0));
    }
  }




  // Lanzamos la simulación
  Simulator::Run ();
  Simulator::Destroy ();

  return observador;
}

//Función que devuelve un número aleatorio que será el servidor asignado.
int aleatorio(uint32_t numServidores)
{
  double min = 0.0;
  double max = numServidores;                                               //el máximo está fuera del intervalo que devuelve por eso sumamos 1.
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (min));
  x->SetAttribute ("Max", DoubleValue (max));

  double value = x->GetValue ();

  return (int) value;
}
