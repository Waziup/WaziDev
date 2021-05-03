pipeline {
  agent {
    docker { image 'ubuntu:latest' }
  }
  stages {
    stage('Build examples') {
      steps {
        sh 'sudo apt-get install arduino-mk'
        sh './make_all.sh'
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
