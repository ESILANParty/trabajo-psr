/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/data-rate.h"
#include "ns3/gnuplot.h"
#include "Observador.h"

using namespace ns3;
#define ALEATORIO 0
#define SECUENCIAL 1

NS_LOG_COMPONENT_DEFINE ("fuentes12");

Observador simulacion (double tasaMediaIn, double tasaMediaOut, uint32_t tamPaqueteIn,
  uint32_t tamPaqueteOut, uint32_t usuariosXbus, uint32_t numRouters,
  uint32_t numServidores, double retardo, uint16_t modoSeleccion);

int seleccionaServidor(uint32_t numServidores, uint16_t modoSeleccion, double * servidor);

int
main (int argc, char *argv[])
{
  GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));
  Time::SetResolution (Time::NS);

  // Intervalos de tiempo en segundos
  double intervaloMedioUsuario = 0.01;
  double intervaloMedioServidor = 0.005;
  // Paquetes en Bytes
  uint32_t tamPaqueteIn = 136;
  uint32_t tamPaqueteOut = 71;
  uint32_t usuariosXbus = 1;
  uint32_t numRouters = 100;
  uint32_t numServidores = 10;
  double retardo = 2;
  uint16_t modoSeleccion = SECUENCIAL;

  // Parametros por linea de comandos
  CommandLine cmd;
  cmd.AddValue ("intervaloMedioUsuario", "Tasa media de entrada en los equipos", intervaloMedioUsuario);
  cmd.AddValue ("intervaloMedioServidor", "Tasa media de salida en los equipos", intervaloMedioServidor);
  cmd.AddValue ("tamPaqueteIn", "Tamaño de paquetes de entrada", tamPaqueteIn);
  cmd.AddValue ("tamPaqueteOut", "Tamaño de paquetes de salida", tamPaqueteOut);
  cmd.AddValue ("usuariosXbus", "Equipos de jugadores por cada bus CSMA", usuariosXbus);
  cmd.AddValue ("numRouters", "Numero de routers", numRouters);
  cmd.AddValue ("numServidores", "Numero de servidores", numServidores);
  cmd.AddValue ("retardo", "Retardo de envio", retardo);
  cmd.AddValue ("modoSelección", "Modo de selección del servidor", modoSeleccion);
  cmd.Parse (argc,argv);

  Observador observador = simulacion (intervaloMedioUsuario, intervaloMedioServidor, tamPaqueteIn,
    tamPaqueteOut, usuariosXbus, numRouters, numServidores, retardo, modoSeleccion);

  NS_LOG_INFO("Porcentaje de paquetes correctos: " << observador.PorcentajeCorrectos ());

  return 0;
}

Observador simulacion (double intervaloMedioUsuario, double intervaloMedioServidor, uint32_t tamPaqueteIn,
  uint32_t tamPaqueteOut, uint32_t usuariosXbus, uint32_t numRouters,
  uint32_t numServidores, double retardo, uint16_t modoSeleccion)
{
  // Variables aleatorias del sistema
  Ptr<NormalRandomVariable> IntervaloUsuario = CreateObject<NormalRandomVariable> ();
  Ptr<NormalRandomVariable> IntervaloServidor = CreateObject<NormalRandomVariable> ();
  IntervaloUsuario->SetAttribute ("Mean", DoubleValue(intervaloMedioUsuario));
  IntervaloServidor->SetAttribute ("Mean", DoubleValue(intervaloMedioServidor));


  // Variable para calcular los estadisticos
  Observador observador;

  // Creacion de los nodos p2p (servidores y gateway)
  NS_LOG_INFO("Creando los nodos p2p de los servidores...");
  NodeContainer p2pServNodes;
  p2pServNodes.Create (numServidores+1);

  // Creacion de los nodos p2p de todos los routers
  NS_LOG_INFO("Creando los buses CMSA de gateways...");
  NodeContainer p2pRoutersNodes;
  p2pRoutersNodes.Add (p2pServNodes.Get (0));
  p2pRoutersNodes.Create (numRouters);

  // Creacion de bus CSMA en cada router con n equipos
  NS_LOG_INFO("Creando los buses CSMA de los equipos usuarios...");
  NodeContainer p2pUsuariosNodes [numRouters];
  for (uint32_t i=0; i<numRouters; i++)
  {
    p2pUsuariosNodes[i].Add (p2pRoutersNodes.Get (i+1));
    p2pUsuariosNodes[i].Create (usuariosXbus);
  }

  // Instalacion de los servidores a los P2P con el router
  // El nodo 0 es el router y el resto los servidores conectados a el
  NS_LOG_INFO("Instalando las conexiones p2p...");
  PointToPointHelper pointToPointServ [numServidores];
  NetDeviceContainer p2pServDevices [numServidores];
  for(uint32_t i=0; i<numServidores; i++)
  {
    pointToPointServ[i].SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
    pointToPointServ[i].SetChannelAttribute ("Delay", StringValue ("1ms"));
    p2pServDevices[i] = pointToPointServ[i].Install (p2pServNodes.Get (0), p2pServNodes.Get (i+1));
  }

  // Instalacion del bus CSMA de routers
  NS_LOG_INFO("Instalando las conexiones del bus CSMA de los gateways...");
  PointToPointHelper pointToPointRouters [numRouters];
  NetDeviceContainer p2pRoutersDevices [numRouters];
  for(uint32_t i=0; i<numRouters; i++)
  {
    pointToPointRouters[i].SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
    pointToPointRouters[i].SetChannelAttribute ("Delay", StringValue ("1ms"));
    p2pRoutersDevices[i] = pointToPointRouters[i].Install (p2pRoutersNodes.Get (0), p2pRoutersNodes.Get (i+1));
  }

  // Instalacion de los buses CSMA de los usuarios conectados a cada router
  NS_LOG_INFO("Instalando las conexiones de los buses CSMA de los equipos usuario con su respectivo gateway...");
  PointToPointHelper pointToPointUsuarios [numRouters*usuariosXbus];
  NetDeviceContainer p2pUsuariosDevices [numRouters];
  for(uint32_t i=0; i<numRouters; i++)
  {
    for(uint32_t j=0; j<usuariosXbus; j++)
    {
      pointToPointUsuarios[i].SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
      pointToPointUsuarios[i].SetChannelAttribute ("Delay", StringValue ("1ms"));
      p2pUsuariosDevices[i].Add (pointToPointUsuarios[i*usuariosXbus+j].Install (p2pUsuariosNodes[i].Get (0), p2pUsuariosNodes[i].Get (j+1)));
    }
  }
  //Capturador de envío y recepción de los equipos usuarios
  for(uint32_t i=0; i<numRouters; i++)
  {
    for(uint32_t j=0; j<usuariosXbus; j++)
    {
      p2pUsuariosDevices[i].Get (j+1)->GetObject<PointToPointNetDevice> ()
      ->TraceConnectWithoutContext ("MacTx", MakeCallback(&Observador::MacTxClient, &observador));
      p2pUsuariosDevices[i].Get (j+1)->GetObject<PointToPointNetDevice> ()
      ->TraceConnectWithoutContext ("MacRx", MakeCallback(&Observador::MacRxClient, &observador));
    }
  }
  // Capturador de envío y recepción de los servidores
  for(uint32_t i=0; i<numServidores; i++)
  {
      p2pServDevices[i].Get (1)->GetObject<PointToPointNetDevice> ()
      ->TraceConnectWithoutContext ("MacTx", MakeCallback(&Observador::MacTxServ, &observador));
      p2pServDevices[i].Get (1)->GetObject<PointToPointNetDevice> ()
      ->TraceConnectWithoutContext ("MacRx", MakeCallback(&Observador::MacRxServ, &observador));
  }

  // Instalamos la pila TCP/IP en todos los nodos
  NS_LOG_INFO("Instalando la pila TCP/IP en todos los nodos...");
  InternetStackHelper stack;
  for(uint32_t i=1; i<=numServidores; i++)
  {
    stack.Install (p2pServNodes.Get (i));
  }
  stack.Install (p2pRoutersNodes);
  for(uint32_t i=0; i<numRouters; i++)
  {
    for(uint32_t j=1; j<=usuariosXbus; j++)
    {
      stack.Install (p2pUsuariosNodes[i].Get (j));
    }
  }

  // Asignamos direcciones a cada una de las interfaces
  NS_LOG_INFO("Asignando direcciones IP,");
  NS_LOG_INFO("a los nodos p2p...");
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer p2pServInterfaces [numServidores];
  for(uint32_t i=0; i<numServidores; i++)
  {
    std::ostringstream network;
    network << "10.1." << (i+1) << ".0";
    address.SetBase (network.str ().c_str (), "255.255.255.0");
    p2pServInterfaces[i] = address.Assign (p2pServDevices[i]);
  }

  NS_LOG_INFO("a los nodos CSMA encargados de hacer de gateways...");
  Ipv4InterfaceContainer p2pRoutersInterfaces;
  for(uint32_t i=0; i<numRouters; i++)
  {
    std::ostringstream network;
    network << "10.1." << (numServidores+1+i) << ".0";
    address.SetBase (network.str ().c_str (), "255.255.255.0");
    p2pRoutersInterfaces = address.Assign (p2pRoutersDevices[i]);
  }

  NS_LOG_INFO("y a los nodos CSMA de los equipos usuario...");
  Ipv4InterfaceContainer p2pUsuariosInterfaces [numRouters];
  for(uint32_t i=0; i<numRouters; i++)
  {
    for(uint32_t j=0; j<usuariosXbus; j++)
    {

    }
    std::ostringstream network;
    network << "10.1." << ((i+1) + (numRouters+numServidores+1)) << ".0";
    address.SetBase (network.str ().c_str (), "255.255.255.0");
    p2pUsuariosInterfaces[i] = address.Assign (p2pUsuariosDevices[i]);
  }

  // Calculamos las rutas del escenario. Con este comando, los
  //     nodos de la red de área local definen que para acceder
  //     al nodo del otro extremo del enlace punto a punto deben
  //     utilizar el primer nodo como ruta por defecto.
  NS_LOG_INFO("Calculando rutas del escenario...");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Creamos las aplicaciones serveridoras UDP en los nodos servidores y en los nodos de los jugadores
  NS_LOG_INFO("Creando las aplicaciones servidor UDP");
  uint16_t port = 6000;
  UdpServerHelper server (port);
  NodeContainer serverNodes;
  for(uint32_t i=1; i<=numServidores; i++)
  {
    serverNodes.Add (p2pServNodes.Get (i));
  }

  for(uint32_t i=0; i<numRouters; i++)
  {
    for(uint32_t j=1; j<=usuariosXbus; j++)
    {
     serverNodes.Add (p2pUsuariosNodes[i].Get (j));
    }
  }
  ApplicationContainer serverApps = server.Install (serverNodes);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  // Creamos las aplicaciones clientes UDP en los nodos clientes eligiendo el servidor de forma aleatoria...
  NS_LOG_INFO("Creando las aplicaciones cliente UDP y de ping");
  Time interPacketInterval = Seconds (0.01);   //... y las aplicaciones clientes en los servidores que han sido elegidos
  uint32_t maxPacketCount = 10000;
  UdpClientHelper client[numRouters*usuariosXbus];
  ApplicationContainer clientApps;
  UdpClientHelper serv[numRouters*usuariosXbus];
  ApplicationContainer pingApps;

  for(uint32_t i=0; i<numRouters; i++)
  {
    for(uint32_t j=0; j<usuariosXbus; j++)
    {
      double servidor = 0;
      servidor = seleccionaServidor(numServidores, modoSeleccion, &servidor);
      client[i*usuariosXbus+j] = UdpClientHelper (p2pServInterfaces[(int)servidor].GetAddress (1), port);
      client[i*usuariosXbus+j].SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      client[i*usuariosXbus+j].SetAttribute ("Interval", TimeValue (interPacketInterval));
      client[i*usuariosXbus+j].SetAttribute ("PacketSize", UintegerValue (tamPaqueteOut));
      clientApps.Add (client[i*usuariosXbus+j].Install (p2pUsuariosNodes[i].Get (j+1)));

      //Ya Conocemos el servidor elegido y el equipo que lo elige, instalamos el cliente en dicho servidor
      //poniendo como servAddress la dirección del equipo que lo ha elegido.
      serv[i*usuariosXbus+j] = UdpClientHelper (p2pUsuariosInterfaces[i].GetAddress (j+1), port);
      serv[i*usuariosXbus+j].SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      serv[i*usuariosXbus+j].SetAttribute ("Interval", TimeValue (interPacketInterval));
      serv[i*usuariosXbus+j].SetAttribute ("PacketSize", UintegerValue (tamPaqueteIn));
      clientApps.Add (serv[i*usuariosXbus+j].Install (p2pServNodes.Get ((int)servidor+1)));
    }
  }
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  NS_LOG_INFO("Habilitando pcap del equipo usuario nº1 del bus csma 1...");
  pointToPointUsuarios[0].EnablePcap ("trabajo-psr", p2pUsuariosDevices[0].Get (1), true);
  pointToPointServ[0].EnablePcap ("trabajo-psr-Serv", p2pServDevices[0].Get (1), true);

  // Lanzamos la simulación
  NS_LOG_INFO("Simulando...");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO("¡Hecho!");

  return observador;
}

//Función que devuelve un número aleatorio que será el servidor asignado.
int seleccionaServidor(uint32_t numServidores, uint16_t modoSeleccion, double * servidor)
{

  if(modoSeleccion == ALEATORIO)
  {
    double min = 0.0;
    double max = numServidores;
    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();           //el máximo está fuera del intervalo que devuelve.
    x->SetAttribute ("Min", DoubleValue (min));
    x->SetAttribute ("Max", DoubleValue (max));
    *servidor = x->GetValue ();
   }
   else
   {
    if(*servidor == numServidores-1 || *servidor == 0)
      *servidor=0;
    else
      *servidor= (*servidor)+1;
   }

  return (int) *servidor;
}
