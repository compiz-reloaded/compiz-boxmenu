def xml_indent(elem, level=0):
	i = "\n" + level*"\t" #used to be "  ", use actual tabs
	if len(elem):
		if not elem.text or not elem.text.strip():
			elem.text = i + "\t"

		e = None
		for e in elem:
			xml_indent(e, level+1)
			if not e.tail or not e.tail.strip():
				e.tail = i + "\t"

		if e is not None and not e.tail or not e.tail.strip():
			e.tail = i
	else:
		if level and (not elem.tail or not elem.tail.strip()):
			elem.tail = i
