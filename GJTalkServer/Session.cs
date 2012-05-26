﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using Matrix.Xml;
using Matrix.Xmpp;
using Matrix.Xmpp.Component;
using Matrix.Xmpp.Sasl;
using Matrix.Xmpp.Stream;
using MxAuth = Matrix.Xmpp.Sasl.Auth;
namespace GJTalkServer
{
    enum SessionOfflineReason
    {
        ServerDown,
        MultiLogin,
        Force
    }
    enum DataSentAction
    {
        Close,
        None
    }
    class Session
    {
        public User SessionUser { get; private set; }
        public bool IsOnline { get { return SessionUser != null; } }

        bool stoped = false;
        GJTalkServer server;
        XmppStreamParser streamParser;
        Socket socket;
        AsyncCallback readCallback;
        byte[] buffer;
        const int buff_size = 1024;

        public Session(GJTalkServer server, Socket socket)
        {
            this.server = server;
            this.socket = socket;
            server.SessionManager.Add(this);
            readCallback = new AsyncCallback(OnRead);

            buffer = new byte[buff_size];
            streamParser = new XmppStreamParser();
            streamParser.OnError += streamParser_OnError;
            streamParser.OnStreamElement += streamParser_OnStreamElement;
            streamParser.OnStreamEnd += streamParser_OnStreamEnd;
            streamParser.OnStreamStart += streamParser_OnStreamStart;
            BeginRead();

        }


        void BeginRead()
        {
            socket.BeginReceive(buffer, 0, buff_size, SocketFlags.None, readCallback, null);
        }
        public void Send(string data)
        {
            Send(data, null);
        }
        public void Send(string data, DataSentAction action)
        {
            Send(data, (object)action);
        }
        public void Send(string data, object userState)
        {
            if (string.IsNullOrEmpty(data))
                return;
            byte[] buffer = Encoding.UTF8.GetBytes(data);
            socket.BeginSend(buffer, 0, buffer.Length,
               SocketFlags.None, OnSent, userState);
        }
        public void Send(XmppXElement element)
        {
            Send(element.ToString());
        }
        public void Send(XmppXElement element, DataSentAction action)
        {
            Send(element.ToString(), action);
        }
        public void Send(XmppXElement element, object userState)
        {
            Send(element.ToString(), userState);
        }
        void OnRead(IAsyncResult result)
        {
            try
            {
                int size = socket.EndReceive(result);
                if (size > 0)
                {
                    streamParser.Write(buffer, 0, size);
                    BeginRead();
                }
                else
                {
                    Close();
                }

            }
            catch
            {
                Close();
            }
        }
        void OnSent(IAsyncResult result)
        {
            try
            {
                int size = socket.EndSend(result);
                if (size == 0)
                {
                    Close();
                    return;
                }
            }
            catch
            {
                Close();
                return;
            }
            if (result.AsyncState != null)
            {
                if (result.AsyncState is DataSentAction)
                {
                    switch ((DataSentAction)result.AsyncState)
                    {
                        case DataSentAction.None:
                            break;
                        case DataSentAction.Close:
                            Close();
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        public void Stop()
        {
            Send("</stream:stream>", DataSentAction.Close);
        }
        public void Close()
        {
            socket.Shutdown(SocketShutdown.Both);
            socket.Close();
        }
        void streamParser_OnStreamStart(object sender, Matrix.StanzaEventArgs e)
        {
            SendStreamHeader();
            Send(BuildFeatures());
        }

        void streamParser_OnStreamEnd(object sender, Matrix.EventArgs e)
        {
            Close();
        }

        void streamParser_OnStreamElement(object sender, Matrix.StanzaEventArgs e)
        {
            server.PacketManager.Enqueue(new Packet()
            {
                Session = this,
                Stanza = e.Stanza,
                State = e.State,
                Server = server
            });
        }

        void streamParser_OnError(object sender, Matrix.ExceptionEventArgs e)
        {
            Stop();
        }

        void SendStreamHeader()
        {
            var stream = new Stream()
            {
                Version = "1.0",
                From = server.Domain,
                Id = Guid.NewGuid().ToString()
            };
            Send(stream.StartTag());
        }
        XmppXElement BuildFeatures()
        {
            var feat = new StreamFeatures();
            var mechs = new Mechanisms();
            mechs.AddMechanism(SaslMechanism.PLAIN);
            feat.Mechanisms = mechs;
            return feat;
        }

        public void Offline(SessionOfflineReason reason)
        {
            server.SessionManager.Remove(this);
            Send(new Message(SessionUser.GetJid())
            {
                Subject = "ServerMessage",
                Body = "Offline:" + reason.ToString()
            });
            this.SessionUser = null;
            Stop();
        }
        public void SetOnline(string username)
        {
            Session oldSession = server.SessionManager.GetSession(username);
            if (oldSession != null)
                oldSession.Offline(SessionOfflineReason.MultiLogin);

            this.SessionUser = server.AuthManager.GetUser(username);

            server.SessionManager.Add(this);
        }

    }

}
