.pragma library

function getCleanPath(path) {
    let cleanPath = path.replace(/^(file:\/{2})/, "");

    if (cleanPath.startsWith("/") && cleanPath.charAt(2) === ":") {
        cleanPath = cleanPath.substring(1);
    }
    return cleanPath;
}

function getFileName(path) {
    return getCleanPath(path).split(/[/\\]/).pop();
}

function isNullOrEmpty(s) {
    return !s;
}

function isNullOrWhitespace(s) {
    return !s || s.trim() === "";
}

function capitalizeModSide(s) {
    switch (s) {
        case "client":
            return qsTr("Client Side")
        case "server":
            return qsTr("Server Side")
        case "both":
            return qsTr("Both")
    }
}