/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1   n2   n3   n4
//    point-to-point  |    |    |    |
//                    ================
//                      LAN 10.1.2.0

#define CURVAS 5
#define VALORES_TIEMPO_ON 5
#define REPETICIONES 13

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Practica06");

Observador simulacion (uint32_t nCsma, double tOnMedio, double tOffMedio,
                        uint32_t sizePkt, DataRate dataRate, uint8_t tamCola);

int
main (int argc, char *argv[])
{
  GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));
  Time::SetResolution (Time::NS);

  // Parametros por defecto
  uint8_t DNI []    = {0, 2};
  uint32_t nCsma    = 50 + 5*DNI[0];
  double tOnMedio   = 0.150;
  double tOffMedio  = 0.650;
  uint32_t sizePkt  = 40 - DNI[1]/2;
  DataRate dataRate = DataRate("64kbps");

  // Parametros por linea de comandos
  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("tOnMedio", "Tiempo medio de on", tOnMedio);
  cmd.AddValue ("tOffMedio", "Tiempo medio de off", tOffMedio);
  cmd.AddValue ("sizePkt", "TamaÃ±o de los paquetes", sizePkt);
  cmd.AddValue ("dataRate", "Tasa de envio en el estado activo", dataRate);
  cmd.Parse (argc,argv);

  // Objetos necesarios para las graficas
  Gnuplot plotPqtsCorrectos;
  Gnuplot plotLatencia;
  GnuplotLatenciadDataset datosPqts[CURVAS];
  GnuplotLatenciadDataset datosLatencia[CURVAS];
  double ts13n95p = 2.1788; // n = 13, 95% -> (12, 0.025)
  double z_PqtsCorrectos;
  double z_RetardoMedio;
  Average<double> avgPorcentajesPqtsCorrectos;
  Average<double> avgRetardosMedios;

  // Valores precargados para las simulaciones
  std::ostringstream titulo;
  titulo << "Simulacion con nCsma = " << nCsma << ", tOnMedio = " << tOnMedio
  << "s, tOffMedio = " << tOffMedio << "s, sizePkt = " << sizePkt << "B y dataRate = "
  << dataRate;
  datosPqts.SetTitle (titulo.str ());
  datosPqts.SetLegend ("NÃºmeros de usuarios", "Paquetes correctos");
  datosLatencia.SetTitle (titulo.str ());
  datosLatencia.SetLegend ("NÃºmeros de usuarios",
  "Latencia (ms)");

  // Bucle para los distintos valores del tamaÃ±o de cola
	for(int i=0; i<CURVAS; i++)
	{
    // Configuracion de las curvas i de cada grafica
    std::ostringstream rotulo;
    rotulo << "TamaÃ±o de cola: " << (i + 1);
    datosPqts[i] = GnuplotLatenciadDataset (rotulo.str ());
    datosPqts[i].SetStyle(GnuplotLatenciadDataset::LINES_POINTS);
	  datosPqts[i].SetErrorBars(GnuplotLatenciadDataset::Y);
    datosLatencia[i] = GnuplotLatenciadDataset (rotulo.str ());
    datosLatencia[i].SetStyle(GnuplotLatenciadDataset::LINES_POINTS);
    datosLatencia[i].SetErrorBars(GnuplotLatenciadDataset::Y);

    // Bucle para los distintos tiempos medios del estado On de las fuentes
    for(int n=0; n<VALORES_TIEMPO_ON; n++)
    {
      NS_LOG_INFO ("Simulando con tamaÃ±o de cola: " << (i + 1) << " y tiempo medio en On: "
                    << tOnMedio << "s");
      // Bucle de simulaciones con los mismos parametros
      for(int k=0; k<REPETICIONES; k++)
      {
        // Hacemos una simulacion simple
        Observador observador = simulacion (nCsma, tOnMedio, tOffMedio, sizePkt, dataRate, (i+1));
        NS_LOG_INFO ("ITERACION " << k << " . % paquetes correctos: " << observador.PorcentajeCorrectos () << "%");
        NS_LOG_INFO ("ITERACION " << k << " . Retardo medio: " << observador.TiempoTxMedio () << "ms");
        // y acumulamos las muestras
        avgPorcentajesPqtsCorrectos.Update (observador.PorcentajeCorrectos ());
        avgRetardosMedios.Update (observador.TiempoTxMedio ());
      }

      // Calculamos el intervalo de confianza del 95%
      z_PqtsCorrectos = ts13n95p*sqrt(avgPorcentajesPqtsCorrectos.Var () / REPETICIONES);
      NS_LOG_INFO ("IC95% de % paquetes correctos: ("
                    << avgPorcentajesPqtsCorrectos.Avg()-z_PqtsCorrectos << ", "
                    << avgPorcentajesPqtsCorrectos.Avg()+z_PqtsCorrectos << ")");
      z_RetardoMedio = ts13n95p*sqrt(avgRetardosMedios.Var () / REPETICIONES);
      NS_LOG_INFO ("IC95% del retardo medio: ("
                    << avgRetardosMedios.Avg()-z_RetardoMedio << ", "
                    << avgRetardosMedios.Avg()+z_RetardoMedio << ")");
      // y aÃ±adimos los datos
      datosPqts[i].Add(tOnMedio, avgPorcentajesPqtsCorrectos.Avg(), z_PqtsCorrectos);
      datosLatencia[i].Add(tOnMedio, avgRetardosMedios.Avg(), z_RetardoMedio);

      // Actualizamos el tiempo medio de On
      tOnMedio = tOnMedio + 0.050;
      // y reseteamos los acumuladores
      avgPorcentajesPqtsCorrectos.Reset ();
      avgRetardosMedios.Reset ();
    }

    // Reseteamos el tiempo medio de On al valor inicial
    tOnMedio = tOnMedio - 0.050 * VALORES_TIEMPO_ON;

    // AÃ±adimos los datos a las graficas
    plotPqtsCorrectos.AddDataset(datosPqts[i]);
  	plotLatencia.AddDataset(datosLatencia[i]);
  }

  // Guardamos las graficas en los ficheros pedidos
  std::ofstream fichero1("practica06-01.plt");
  plotPqtsCorrectos.GenerateOutput(fichero1);
  fichero1 << "pause -1" << std::endl;
  fichero1.close();
  std::ofstream fichero2("practica06-02.plt");
  plotLatencia.GenerateOutput(fichero2);
  fichero2 << "pause -1" << std::endl;
  fichero2.close();

  return 0;
}

Observador
simulacion (uint32_t nCsma, double tOnMedio, double tOffMedio, uint32_t sizePkt,
            DataRate dataRate, uint8_t tamCola)
{
  // Variables del sistema
  Ptr<ExponentialRandomVariable> ton = CreateObject<ExponentialRandomVariable> ();
  Ptr<ExponentialRandomVariable> toff = CreateObject<ExponentialRandomVariable> ();
  Observador observador;

  // AÃ±adimos los valores medios a las variables aleatorias
  ton->SetAttribute ("Mean", DoubleValue(tOnMedio));
  toff->SetAttribute ("Mean", DoubleValue(tOffMedio));

  nCsma = nCsma == 0 ? 1 : nCsma;

  // Nodos que pertenecen al enlace punto a punto
  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  // Nodos que pertenecen a la red de Ã¡rea local
  // Como primer nodo aÃ±adimos el encaminador que proporciona acceso
  //      al enlace punto a punto.
  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  // Instalamos el dispositivo en los nodos punto a punto
  PointToPointHelper pointToPoint;
  NetDeviceContainer p2pDevices;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  p2pDevices = pointToPoint.Install (p2pNodes);

  // Configuramos la cola de la puerta de enlace (n1)
  p2pDevices.Get (1)->GetObject<PointToPointNetDevice> ()
  ->GetQueue ()->GetObject<DropTailQueue> ()->
  SetAttribute ("MaxPackets", UintegerValue (tamCola));

  // Instalamos el dispositivo de red en los nodos de la LAN
  CsmaHelper csma;
  NetDeviceContainer csmaDevices;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
  csmaDevices = csma.Install (csmaNodes);

  // Bucle para aÃ±adir todas las trazas necesarias a todos los nodos csma
  for (uint32_t i=0; i<=nCsma; i++)
  {
    // Capturador de peticion de envio de un paquete de la capa de aplicacion a la de enlace
    csmaDevices.Get(i)->GetObject<CsmaNetDevice>()
  	->TraceConnectWithoutContext ("MacTx", MakeCallback(&Observador::MacTxFired, &observador));
  }

	// Capturador de envio a la capa de apliacion de un paquete recibido en n0
	p2pDevices.Get(0)->GetObject<PointToPointNetDevice>()
	->TraceConnectWithoutContext ("MacRx", MakeCallback(&Observador::MacRxFiredN0, &observador));

  // Instalamos la pila TCP/IP en todos los nodos
  InternetStackHelper stack;
  stack.Install (p2pNodes.Get (0));
  stack.Install (csmaNodes);

  // Asignamos direcciones a cada una de las interfaces
  // Utilizamos dos rangos de direcciones diferentes:
  //    - un rango para los dos nodos del enlace
  //      punto a punto
  //    - un rango para los nodos de la red de Ã¡rea local.
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer p2pInterfaces;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  p2pInterfaces = address.Assign (p2pDevices);
  Ipv4InterfaceContainer csmaInterfaces;
  address.SetBase ("10.1.2.0", "255.255.255.0");
  csmaInterfaces = address.Assign (csmaDevices);

  // Calculamos las rutas del escenario. Con este comando, los
  //     nodos de la red de Ã¡rea local definen que para acceder
  //     al nodo del otro extremo del enlace punto a punto deben
  //     utilizar el primer nodo como ruta por defecto.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  // Establecemos un sumidero para los paquetes en el puerto 9 del nodo
  //     aislado del enlace punto a punto
  uint16_t port = 9;
  PacketSinkHelper sink ("ns3::UdpSocketFactory",
                         Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
  ApplicationContainer app = sink.Install (p2pNodes.Get (0));

  // Instalamos un cliente OnOff en uno de los equipos de la red de Ã¡rea local
  // con los parametros por defecto o pasados por linea de comandos
  OnOffHelper clientes ("ns3::UdpSocketFactory",
                        Address (InetSocketAddress (p2pInterfaces.GetAddress (0), port)));
  clientes.SetAttribute ("DataRate", DataRateValue(dataRate));
  clientes.SetAttribute ("PacketSize", UintegerValue(sizePkt));
  clientes.SetAttribute ("OnTime", PointerValue(ton));
  clientes.SetAttribute ("OffTime", PointerValue(toff));

  ApplicationContainer clientApps = clientes.Install (csmaNodes);
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (30.0));

  // Lanzamos la simulaciÃ³n
  Simulator::Run ();
  Simulator::Destroy ();

  return observador;
}
