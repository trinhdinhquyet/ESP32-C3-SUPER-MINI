#!/bin/bash
###########################################
# DWM API Server - Linux Deploy Script
###########################################

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration - Modify these paths as needed
TOMCAT_HOME="/opt/tomcat"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
WAR_FILE="dwm-api-server.war"

echo ""
echo "╔═══════════════════════════════════════════════╗"
echo "║   DWM API Server - Deployment Script         ║"
echo "╚═══════════════════════════════════════════════╝"
echo ""

# Check if Maven is installed
if ! command -v mvn &> /dev/null; then
    echo -e "${RED}❌ Maven is not installed!${NC}"
    exit 1
fi

echo "[1/5] Cleaning previous build..."
cd "$PROJECT_DIR"
mvn clean
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Clean failed!${NC}"
    exit 1
fi

echo ""
echo "[2/5] Building WAR file..."
mvn package -DskipTests
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Build failed!${NC}"
    exit 1
fi

echo ""
echo "[3/5] Stopping Tomcat..."
if [ -f "$TOMCAT_HOME/bin/shutdown.sh" ]; then
    sudo "$TOMCAT_HOME/bin/shutdown.sh"
    sleep 5
else
    echo -e "${YELLOW}⚠️ Tomcat not found at $TOMCAT_HOME${NC}"
fi

echo ""
echo "[4/5] Deploying WAR to Tomcat..."
if [ -d "$TOMCAT_HOME/webapps" ]; then
    sudo cp "target/$WAR_FILE" "$TOMCAT_HOME/webapps/"
    echo -e "${GREEN}✅ WAR deployed successfully!${NC}"
else
    echo -e "${RED}❌ Tomcat webapps directory not found!${NC}"
    exit 1
fi

echo ""
echo "[5/5] Starting Tomcat..."
if [ -f "$TOMCAT_HOME/bin/startup.sh" ]; then
    sudo "$TOMCAT_HOME/bin/startup.sh"
    echo -e "${GREEN}✅ Tomcat started!${NC}"
else
    echo -e "${RED}❌ Tomcat startup script not found!${NC}"
fi

echo ""
echo "╔═══════════════════════════════════════════════╗"
echo "║   ✅ Deployment Complete!                     ║"
echo "║                                               ║"
echo "║   Access: http://localhost:8080/${WAR_FILE%.war}/      ║"
echo "║   Logs:   $TOMCAT_HOME/logs/           ║"
echo "╚═══════════════════════════════════════════════╝"
echo ""

