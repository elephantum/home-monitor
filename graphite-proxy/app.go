package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net"
	"os"
	"strconv"
	"time"

	"github.com/marpaia/graphite-golang"
)

func connectGraphite() (*graphite.Graphite, error) {
	if os.Getenv("TEST") == "1" {
		return graphite.NewGraphiteNop("", 0), nil
	}

	graphiteHost := os.Getenv("GRAPHITE_HOST")
	graphitePort, err := strconv.Atoi(os.Getenv("GRAPHITE_PORT"))

	if err != nil {
		graphitePort = 2003
	}

	return graphite.NewGraphite(graphiteHost, graphitePort)
}

func handleRequest(conn net.Conn, graphiteServer *graphite.Graphite) {
	log.Println("Handling request")
	buf := make([]byte, 1024)
	nBytes, err := conn.Read(buf)

	if err != nil {
		fmt.Println("Error reading:", err.Error())
	}

	var data map[string]float64

	err = json.Unmarshal(buf[:nBytes], &data)

	if err != nil {
		fmt.Println("Failed to read JSON:", err.Error())
	}

	log.Println("received", nBytes, "bytes, data:", data)
	conn.Close()

	var metrics []graphite.Metric
	for m, v := range data {
		metrics = append(metrics, graphite.NewMetric(m, strconv.FormatFloat(v, 'f', -1, 64), time.Now().Unix()))
	}

	log.Println(metrics)

	graphiteServer.SendMetrics(metrics)
}

func main() {
	graphiteServer, err := connectGraphite()

	if err != nil {
		log.Println("cannot connect to graphite")
		os.Exit(1)
	}

	listenPort := os.Getenv("PORT")

	if listenPort == "" {
		log.Println("PORT is not set")
		os.Exit(1)
	}

	l, err := net.Listen("tcp", ":"+listenPort)
	defer l.Close()

	if err != nil {
		log.Println("failed to listen :8080")
		os.Exit(1)
	}

	for {
		conn, err := l.Accept()

		if err != nil {
			log.Println("Failed to accept request: ", err.Error())
		} else {
			go handleRequest(conn, graphiteServer)
		}
	}
}
