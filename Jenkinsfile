pipeline {
  agent { 
    docker {image 'arduino/arduino-cli' } 
  }
  environment {
    ARDUINO_DIRECTORIES_USER = "$WORKSPACE"
  }
  stages {
    stage('Prepare') {
      steps {
        sh 'arduino-cli core install arduino:avr'
        sh 'arduino-cli compile -p /dev/ttyUSB0 --fqbn arduino:avr:pro examples/LoRaWAN/Actuation/Actuation.ino'
      }
    }
  }
  post {
    always {
      junit 'tests/report.xml'
    }
    success {
      echo 'Success!'
    }
    failure {
      echo 'Failure!'
    }
    unstable {
      echo 'Unstable'
    }
  }
}
