// *********************************************************** //
// Function for parsing one line from TEXTMAP lump             //
// *********************************************************** //

TextMapLineType parse_next_line(char *mapdata, bool inside_entity, int *position,
								char **key, char **str_value, int *int_value, float *float_value)
{
	char *line = mapdata + *position;
	char *line_end = strchr(line, '\n');
	int next_line_pos = *position + (line_end - line) + 1;
	*position = next_line_pos;
	*float_value = 0.0;
	if (line == line_end)
		return LN_NONE;
	if (!inside_entity)
	{
		if (line[0] == '{')
			return LN_START;
		else if (strncmp(line, "thing", 5) == 0)
			return ENT_THING;
		else if (strncmp(line, "vertex", 6) == 0)
			return ENT_VERTEX;
		else if (strncmp(line, "linedef", 7) == 0)
			return ENT_LINEDEF;
		else if (strncmp(line, "sidedef", 7) == 0)
			return ENT_SIDEDEF;
		else if (strncmp(line, "sector", 6) == 0)
			return ENT_SECTOR;
		else if (strncmp(line, "namespace", 9) == 0)
			return LN_NAMESPACE;
		return LN_NONE;
	}
	// Inside entity definition
	if (line[0] == '}')
		return LN_END;
	// Get key and terminate it with zero byte
	*key = line;
	char *key_end = strchr(line, ' ');
	*key_end = '\0';
	// Get value part, determine its type and get real value
	char *value = key_end + 3;
	if (value[0] == '"')
	{
		// String value
		*str_value = value + 1;
		line_end[-2] = '\0';
		return VAL_STRING;
	}
	else if (strncmp(value, "true;", 5) == 0)
	{
		// Bool value
		*int_value = 1;
		return VAL_BOOL;
	}
	// Integer or float value
	line_end[-1] = '\0';
	char *frac = strchr(value, '.');
	if (frac == NULL)
	{
		// Integer value
		*int_value = atoi(value);
		*float_value = *int_value;
		return VAL_INT;
	}
	// Float value. Get integral part and round according to frac part.
	*float_value = atof(value);
	*frac = '\0';
	int int_part = atoi(value);
	// Round integral part according to fractional part
	if (frac[1] >= '5')
	{
		if (value[0] == '-')
			int_part--;
		else
			int_part++;
	}
	*int_value = int_part;
	int frac_part = atoi(frac + 1);
	return frac_part?VAL_FLOAT:VAL_INT;
}
