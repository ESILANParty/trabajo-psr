/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include "Observador.h"

NS_LOG_COMPONENT_DEFINE ("Observador");


Observador::Observador ()
{
  NS_LOG_FUNCTION_NOARGS ();

  // Inicializamos las variables
  m_totalPaquetesTx     = 0;
  m_totalPaquetesRx     = 0;
}

void
Observador::MacTxClient (Ptr<const Packet> packet)
{
  // Guardamos el momento de envio
  m_tiempoDesdeClient[packet->GetUid ()] = Simulator::Now ();
  NS_LOG_INFO ("Paquete con UID " << packet->GetUid () << " ha sido Tx a los "
                << Simulator::Now ().GetNanoSeconds () / 1000000
                << "ms");
  // y acumulamos el total de paquetes transmitidos
  m_totalPaquetesTx++;
}

void
Observador::MacRxClient (Ptr<const Packet> packet)
{
  std::map<uint64_t, Time>::iterator it = m_tiempoDesdeServ.find (packet->GetUid ());
  if(it != m_tiempoDesdeServ.end ())
  {
    // Guardamos el tiempo de llegada al nodo 0 del enlace punto a punto
    NS_LOG_INFO ("Tiempo de Tx (ms) = " << it->second.GetNanoSeconds () / 1000000);
    m_avgTiemposDesdeServ.Update ((Simulator::Now ().GetNanoSeconds () - it->second.GetNanoSeconds ()) / 1000000);
    NS_LOG_INFO ("Paquete con UID " << packet->GetUid () << " ha completado la Tx en "
                  << (Simulator::Now () - it->second).GetNanoSeconds () / 1000000
                  << "ms");
    // borramos el tiempo de tx
    m_tiempoDesdeServ.erase (it);
    // y acumulamos el paquete recibido
    m_totalPaquetesRx++;
  }
}

void
Observador::MacTxServ (Ptr<const Packet> packet)
{
  // Guardamos el momento de envio
  m_tiempoDesdeServ[packet->GetUid ()] = Simulator::Now ();
  NS_LOG_INFO ("Paquete con UID " << packet->GetUid () << " ha sido Tx a los "
                << Simulator::Now ().GetNanoSeconds () / 1000000
                << "ms");
  // y acumulamos el total de paquetes transmitidos
  m_totalPaquetesTx++;
}

void
Observador::MacRxServ (Ptr<const Packet> packet)
{
  std::map<uint64_t, Time>::iterator it = m_tiempoDesdeClient.find (packet->GetUid ());
  if(it != m_tiempoDesdeClient.end ())
  {
    // Guardamos el tiempo de llegada al nodo 0 del enlace punto a punto
    NS_LOG_INFO ("Tiempo de Tx (ms) = " << it->second.GetNanoSeconds () / 1000000);
    m_avgTiemposDesdeClient.Update ((Simulator::Now ().GetNanoSeconds () - it->second.GetNanoSeconds ()) / 1000000);
    NS_LOG_INFO ("Paquete con UID " << packet->GetUid () << " ha completado la Tx en "
                  << (Simulator::Now () - it->second).GetNanoSeconds () / 1000000
                  << "ms");
    // borramos el tiempo de tx
    m_tiempoDesdeClient.erase (it);
    // y acumulamos el paquete recibido
    m_totalPaquetesRx++;
  }
}

double
Observador::TiempoDesdeClientMedio ()
{
  // Devolvemos la media de las muestras si hay al menos una, si no, cero
  if(m_avgTiemposDesdeClient.Count () > 0)
  {
    return m_avgTiemposDesdeClient.Mean ();
  }
  else
  {
    return 0;
  }
}

double
Observador::TiempoDesdeServMedio ()
{
  // Devolvemos la media de las muestras si hay al menos una, si no, cero
  if(m_avgTiemposDesdeServ.Count () > 0)
  {
    return m_avgTiemposDesdeServ.Mean ();
  }
  else
  {
    return 0;
  }
}

double
Observador::PorcentajeCorrectos ()
{
  return ((double)m_totalPaquetesRx / m_totalPaquetesTx) * 100;
}
