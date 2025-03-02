package main

import (
	"fmt"
	"io"
)

func emit(f file, out io.Writer) error {
	write := func(msg string, args ...any) {
		_, _ = fmt.Fprintf(out, msg, args...)
		_, _ = fmt.Fprintf(out, "\n")
	}

	write("// Code generated by yt-gen-client. DO NOT EDIT.")
	write("")
	write("package internal")

	write("import (")
	write("%q", "go.ytsaurus.tech/yt/go/guid")
	write("%q", "go.ytsaurus.tech/yt/go/yson")
	write("%q", "go.ytsaurus.tech/yt/go/ypath")
	write("%q", "go.ytsaurus.tech/yt/go/yt")
	write("%q", "go.ytsaurus.tech/library/go/core/log")
	write(")")

	for _, opt := range f.options {
		write("func write%s(w *yson.Writer, o *yt.%s) {", opt.name, opt.name)

		write("if o == nil {")
		write("return")
		write("}")

		for _, field := range opt.fields {
			if field.omitnil {
				write("if o.%s != nil {", field.fieldName)
			}

			write("w.MapKeyString(%q)", field.httpName)
			write("w.Any(o.%s)", field.fieldName)

			if field.omitnil {
				write("}")
			}
		}

		for _, name := range opt.embedded {
			write("write%s(w, o.%s)", name, name)
		}

		write("}")
		write("")
	}

	for _, iface := range f.clients {
		for _, m := range iface.methods {
			write("type %sParams struct {", m.name)
			write("verb Verb")
			for i := 0; i < len(m.httpParams); i++ {
				write("%s %s", m.params[i].name, m.params[i].typ)
			}
			write("options *yt.%sOptions", m.name)
			write("}")

			write("func New%sParams(", m.name)
			for i, p := range m.params {
				if m.extra && i+1 == len(m.params) {
					break
				}

				write("%s %s,", p.name, p.typ)
			}
			write("options *yt.%sOptions,", m.name)
			write(") *%sParams {", m.name)
			write("if options == nil {")
			write("options = &yt.%sOptions{}", m.name)
			write("}")
			write("optionsCopy := *options")
			write("return &%sParams{", m.name)
			write("Verb(%q),", m.httpVerb)
			for i := 0; i < len(m.httpParams); i++ {
				write("%s,", m.params[i].name)
			}
			write("&optionsCopy,")
			write("}")
			write("}")
			write("")

			write("func (p *%sParams) HTTPVerb() Verb {", m.name)
			write("return p.verb")
			write("}")

			pathArgName := ""
			for _, p := range m.params {
				switch p.name {
				case "path", "src", "target":
					pathArgName = p.name
				}
			}

			write("func (p *%sParams) YPath() (ypath.YPath, bool) {", m.name)
			if pathArgName != "" {
				write("return p.%s, true", pathArgName)
			} else {
				write("return nil, false")
			}
			write("}")

			write("func (p *%sParams) Log() []log.Field {", m.name)
			write("return []log.Field{")
			for i := 0; i < len(m.httpParams); i++ {
				if m.params[i].name == "spec" {
					continue
				}

				write("log.Any(%q, p.%s),", m.params[i].name, m.params[i].name)
			}
			write("}")
			write("}")
			write("")

			write("func (p *%sParams) MarshalHTTP(w *yson.Writer) {", m.name)
			for i := 0; i < len(m.httpParams); i++ {
				write("w.MapKeyString(%q)", m.httpParams[i])
				write("w.Any(p.%s)", m.params[i].name)
			}

			write("write%sOptions(w, p.options)", m.name)

			write("}")
			write("")

			for _, opt := range f.options {
				if opt.name != m.name+"Options" {
					continue
				}

				for _, embedded := range opt.embedded {
					write("func (p *%sParams) %s() **yt.%s {", m.name, embedded, embedded)
					write("return &p.options.%s", embedded)
					write("}")
					write("")
				}
			}
			write("")
		}
	}

	return nil
}
