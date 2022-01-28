package main

import (
    "encoding/xml"
    "fmt"
    "bytes"
    "io/ioutil"
    "os"
    "flag"
)

var fileName = flag.String("filename", "", "Input Your File Name")
var quiet = flag.Bool("quiet", false, "toggle quiet output") 

type GtkWindow struct {
    Class string
    Id string
}

type Node struct {
	XMLName xml.Name
	Attrs   []xml.Attr `xml:"-"`
	Content []byte     `xml:",innerxml"`
	Nodes   []Node     `xml:",any"`
}

func (n *Node) UnmarshalXML(d *xml.Decoder, start xml.StartElement) error {
	n.Attrs = start.Attr
	type node Node

	return d.DecodeElement((*node)(n), &start)
}

func walk(nodes []Node, f func(Node) bool) {
	for _, n := range nodes {
		if f(n) {
			walk(n.Nodes, f)
		}
	}
}

func main() {
    flag.Parse()
    if *fileName == "" {
        fmt.Println("Usage: go run glade_code_gen.go --filename=./ime_setup.glade --quiet=true")
        return
    }
    if !*quiet {
        fmt.Println("filename " + *fileName)
    }
    file, err := os.Open(*fileName)
    if err!= nil {
        fmt.Printf("failed to open file " + *fileName)
        return
    }
    defer file.Close()

    data, err := ioutil.ReadAll(file)
    if err!=nil {
        fmt.Printf("failed to read file " + *fileName)
        return
    }

    buf := bytes.NewBuffer(data)
	dec := xml.NewDecoder(buf)
    var n Node
	err = dec.Decode(&n)
	if err != nil {
		panic(err)
	}

    var gtk_windows []GtkWindow
	walk([]Node{n}, func(n Node) bool {
		if n.XMLName.Local == "object" {
            //fmt.Println(n.Attrs)
            var wind GtkWindow
            for _, attr := range n.Attrs {
//                fmt.Println("name:" + attr.Name.Local + ", val:"+attr.Value)
                if attr.Name.Local == "class" {
                    wind.Class = attr.Value
                }
                if attr.Name.Local == "id" {
                    wind.Id = attr.Value
                }
            }
            gtk_windows = append(gtk_windows, wind)
		}
		return true
	})

    if !*quiet {
        fmt.Println("\noutput:\n\n```cpp")
    }

    for _, wind := range gtk_windows {
//        fmt.Println("class:" + wind.Class + ", id:"+ wind.Id)
        if wind.Id != "" {
            var class = wind.Class[3:]
            fmt.Println("Gtk::"+class +" *"+wind.Id+";")
            fmt.Println("builder->get_widget<Gtk::"+class+">(\""+wind.Id+"\", " + wind.Id+");");
        }
    }

    if !*quiet {
        fmt.Println("```")
    }


}
