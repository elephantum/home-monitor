package main

import (
	"errors"
	"log"
	"net/http"
	"net/url"
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

	if graphiteHost == "" {
		return nil, errors.New("GRAPHITE_HOST is not set")
	}

	if err != nil {
		graphitePort = 2003
	}

	return graphite.NewGraphite(graphiteHost, graphitePort)
}

func handleRequest(graphiteServer *graphite.Graphite, w http.ResponseWriter, req *http.Request) {
	log.Println("Handling request")

	parsedURL, err := url.ParseRequestURI(req.RequestURI)

	if err != nil {
		log.Println("Failed to parse request:", req.RequestURI)
		return
	}

	params := parsedURL.Query()

	log.Println("Received request:", params)

	var metrics []graphite.Metric
	for m := range params {
		metrics = append(metrics, graphite.NewMetric(m, params.Get(m), time.Now().Unix()))
	}

	log.Println(metrics)

	graphiteServer.SendMetrics(metrics)
}

func main() {
	graphiteServer, err := connectGraphite()

	if err != nil {
		log.Println("cannot connect to graphite:", err.Error())
		os.Exit(1)
	}

	listenPort := os.Getenv("PORT")

	if listenPort == "" {
		log.Println("PORT is not set")
		os.Exit(1)
	}

	graphiteHandler := func(w http.ResponseWriter, req *http.Request) {
		handleRequest(graphiteServer, w, req)
	}

	http.HandleFunc("/", graphiteHandler)

	err = http.ListenAndServe(":"+listenPort, nil)

	if err != nil {
		log.Println("Failed to start HTTP:", err.Error())
		os.Exit(1)
	}
}
