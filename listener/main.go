package main

import (
	"database/sql"
	"flag"
	"fmt"
	"log"
	"net"
	"os"
	"os/signal"
	"strings"
	"syscall"

	_ "github.com/mattn/go-sqlite3"
)

func splitArguments(message string) map[string]string {
	args := make(map[string]string)
	for _, elem := range strings.SplitN(message, "|", 9) {
		parts := strings.SplitN(elem, "=", 2)
		if len(parts) == 2 {
			args[strings.ToLower(parts[0])] = parts[1]
		}
	}
	return args
}

func processMessages(messageChan <-chan string, db *sql.DB, done chan<- struct{}) {
	defer close(done)

	stmt, err := db.Prepare(`insert into fingerprints
		(jobidraw, stepid, pid, hash, host, time, layer, info, content)
		values (?, ?, ?, ?, ?, ?, ?, ?, ?)`)
	if err != nil {
		log.Fatalf("prepare insert: %v", err)
	}
	defer stmt.Close()

	for message := range messageChan {
		args := splitArguments(message)
		if _, err := stmt.Exec(
			args["jobidraw"],
			args["stepid"],
			args["pid"],
			args["hash"],
			args["host"],
			args["time"],
			args["layer"],
			args["type"],
			args["content"],
		); err != nil {
			log.Printf("insert: %v", err)
		}
	}
}

func readLoop(pc net.PacketConn, messageChan chan<- string) uint64 {
	var dropped uint64
	buf := make([]byte, 64*1024)
	for {
		n, _, err := pc.ReadFrom(buf)
		if err != nil {
			return dropped
		}
		select {
		case messageChan <- string(buf[:n]):
		default:
			dropped++
		}
	}
}

func main() {
	dbPath := flag.String("db", "./siren.db", "Path to the SQLite database file")
	incomingPort := flag.String("port", ":4444", "UDP listen address (host:port, e.g. :4444)")
	flag.Parse()

	fmt.Fprintf(os.Stderr, "dbListener: db=%s port=%s\n", *dbPath, *incomingPort)

	db, err := sql.Open("sqlite3", *dbPath)
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()

	// Wire format: JOBIDRAW=..|STEPID=..|PID=..|HASH=..|HOST=..|TIME=..|LAYER=..|TYPE=..|CONTENT=..
	if _, err := db.Exec(`create table if not exists fingerprints (
		jobidraw integer not null,
		stepid   integer not null,
		pid      integer not null,
		hash     text    not null,
		host     text    not null,
		time     integer not null,
		layer    text    not null,
		info     text    not null,
		content  text    not null)`); err != nil {
		log.Fatalf("create table: %v", err)
	}

	pc, err := net.ListenPacket("udp", *incomingPort)
	if err != nil {
		log.Fatal(err)
	}

	messageChan := make(chan string, 5000)
	processorDone := make(chan struct{})
	go processMessages(messageChan, db, processorDone)

	sigs := make(chan os.Signal, 1)
	signal.Notify(sigs, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		sig := <-sigs
		fmt.Fprintf(os.Stderr, "dbListener: received %s, shutting down\n", sig)
		pc.Close()
	}()

	dropped := readLoop(pc, messageChan)

	close(messageChan)
	<-processorDone
	if dropped > 0 {
		fmt.Fprintf(os.Stderr, "dbListener: dropped %d messages (channel full)\n", dropped)
	}
	fmt.Fprintln(os.Stderr, "dbListener: shutdown complete")
}
