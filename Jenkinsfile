pipeline {
  //agent { 
  //  docker {image 'arduino/arduino-cli' } 
  //}
  //agent { dockerfile true }
  agent any
  environment {
    ARDUINO_DIRECTORIES_USER = "$WORKSPACE"
  }
  stages {
    stage('Prepare') {
      steps {
        sh 'brew update'
        sh 'brew install arduino-cli'
        sh 'arduino-cli core update-index'
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
