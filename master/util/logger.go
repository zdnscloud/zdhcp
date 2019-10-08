package util

import (
	"cement/log"
)

var glogger log.Logger

func InitLog(path, level string) {
	var logLevel log.LogLevel
	switch level {
	case "debug":
		logLevel = log.Debug
	case "warn":
		logLevel = log.Warn
	case "info":
		logLevel = log.Info
	case "error":
		logLevel = log.Error
	default:
		panic("unknown log level:" + string(level))
	}

	if path == "" {
		glogger = log.NewLog4jConsoleLogger(logLevel)
	} else {
		var err error
		glogger, err = log.NewLog4jLogger(path, logLevel, 0, 0)
		if err != nil {
			panic("create log4jLogger error:" + err.Error())
		}
	}
}

func Logger() log.Logger {
	return glogger
}
