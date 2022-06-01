function joao_format(str)
{
    //KEYWORDS
    let keyword_color = '#f00';
    str = str.replace(/^\/main/gm,"/<font color='" + keyword_color + "'>main</font>"); // Snowflake thing for main since it needs the slash
    let keywords = ["if", "elseif","else","for","while", "throw","try","catch"];
    for(index in keywords)
    {
        let key = keywords[index];
        let reg = RegExp("\\b" + key + "\\b","gm");
        str = str.replace(reg,"<font color='" + keyword_color + "'>" + key + "</font>");
    }

    //TYPES
    let type_color = '#e88';
    let types = [ "Number", "Object", "String","Value","table","file"];
    for(index in types)
    {
        let type = types[index];
        let reg = RegExp("\\b" + type + "\\b","gm");
        str = str.replace(reg,"<font color='" + type_color + "'>" + type + "</font>");
    }

    //FUNCTIONS
    let function_color = '#42c2ff';
    let functions = ['print','New'];
    for(index in functions)
    {
        let func = functions[index];
        let reg = RegExp("\\b" + func + "\\b","gm");
        str = str.replace(reg,"<font color='" + function_color + "'>" + func + "</font>");
    }

    //STRINGS
    let str_color = '#f55';
    str = str.replace(/"(.*?)"/gm,"<font color='" + str_color + "'>\"$1\"</font>");

    //COMMENTS
    let comment_color = '#88da88';
    str = str.replace(/(\#\#.*)$/gm, "<font color='" + comment_color +"'>$1</font>")

    return str;
};

let codes = document.getElementsByClassName("joaocode");

for(index in codes)
{
    let element = codes[index];
    if(element.innerHTML)
    {
        element.innerHTML = joao_format(element.innerHTML);
    }
}