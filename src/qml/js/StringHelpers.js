.pragma library

function getFileName(path) {
    let cleanPath = path.replace(/^(file:\/{2})/, "");

    if (cleanPath.startsWith("/") && cleanPath.charAt(2) === ":") {
        cleanPath = cleanPath.substring(1);
    }

    return cleanPath.split(/[/\\]/).pop();
}